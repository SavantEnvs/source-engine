#!/usr/bin/env bash
# mayhem/build.sh — build source-engine's low-level libraries (tier0/1/2, vstdlib, mathlib, ...) with
# upstream's waf build (the `--tests` project set), twice:
#   1. build-test/   : NORMAL flags, upstream's own unit tests (utils/unittest + unittests/*) for test.sh
#   2. build-fuzz/   : $SANITIZER_FLAGS + SanCov, the libraries the fuzz harnesses link
# then link each mayhem/harnesses/fuzz_*.cpp as a libFuzzer binary (/mayhem/<name>) and a standalone
# reproducer (/mayhem/<name>-standalone). All build dirs match upstream's `build*/` .gitignore entry.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${OUT:=$SRC}"
export CC CXX

cd "$SRC"

# UBSan relaxations (ASan and every other UBSan check stay on and halting):
#  - float-cast-overflow: tier0's static CPU-clock calibration (tier0/cpu_posix.cpp:149) divides by a
#    near-zero time delta in containers and trips it at process start, before any input. (The fuzz
#    binaries also link mayhem/cpufreq_env.cc, which keeps that calibration from running at all.)
#  - enum: KeyValues/kvpacker read a raw byte straight into their type enums (KeyValues.cpp:2727/2805,
#    kvpacker.cpp:181/274) and then range-check it with a switch/compare; UBSan's invalid-enum-load
#    check aborts on that load for almost every mutated binary input, before the parser's own check.
SAN="$SANITIZER_FLAGS"
case "$SAN" in *undefined*) SAN="$SAN -fno-sanitize=float-cast-overflow -fno-sanitize=enum" ;; esac

waf_build() { # <outdir> <prefix> ; flags come from CFLAGS/CXXFLAGS/LINKFLAGS in the env
  python3 ./waf configure -T release --disable-warns --tests -o "$1" --prefix="$2" >/dev/null
  python3 ./waf install -j"$MAYHEM_JOBS"
}

# 1) Upstream unit tests, normal flags.
env -u CFLAGS -u CXXFLAGS -u LDFLAGS \
  CFLAGS="$COVERAGE_FLAGS" CXXFLAGS="$COVERAGE_FLAGS" LINKFLAGS="$COVERAGE_FLAGS" \
  bash -c "$(declare -f waf_build); MAYHEM_JOBS=$MAYHEM_JOBS; waf_build build-test build-test/install"

# 2) Sanitized + coverage-instrumented libraries. DEBUG_FLAGS last so -gdwarf-3 wins.
#    SanCov callbacks are resolved by the sanitizer runtime (the waf build also links upstream's unit
#    test executables), so with an explicitly EMPTY SANITIZER_FLAGS the libraries are built without
#    SanCov: that variant is a plain reproducer build, not a fuzzing one.
COV=""
[ -n "$SAN" ] && COV="-fsanitize=fuzzer-no-link"
FUZZ_CFLAGS="$SAN $COV $DEBUG_FLAGS"
CFLAGS="$FUZZ_CFLAGS" CXXFLAGS="$FUZZ_CFLAGS" LINKFLAGS="$SAN $COV" waf_build build-fuzz build-fuzz/install

# 3) Harnesses. Reuse the exact DEFINES waf compiled the libraries with.
B="$SRC/build-fuzz"
DEFS=$(python3 - "$B/c4che/_cache.py" <<'PY'
import ast, sys
for line in open(sys.argv[1]):
    if line.startswith('DEFINES = '):
        print(' '.join('-D' + d.replace('"', '') for d in ast.literal_eval(line.split('=', 1)[1].strip())))
PY
)
: "${STB_DIR:=/opt/toolchains/deps/stb-c5b901ecef515ea068fa8b8a19ca5cd5353905cb/src}"
INC="-I$STB_DIR -I$SRC/common -I$SRC/public -I$SRC/public/tier0 -I$SRC/public/tier1 -I$SRC/public/tier2 -I$SRC/mayhem/harnesses -I$SRC/game/shared -I$SRC/utils/common"
HFLAGS="$SAN $DEBUG_FLAGS -O1 -std=c++11 -fpermissive -w -fno-strict-aliasing -DTIER1_STATIC_LIB=1 $DEFS $INC"
LIBS="$B/tier3/libtier3.a $B/tier2/libtier2.a $B/tier1/libtier1.a $B/mathlib/libmathlib.a -L$B/tier0 -L$B/vstdlib -ltier0 -lvstdlib -Wl,-rpath,$B/tier0:$B/vstdlib -ldl -lm -pthread"

# dmxloader, bitmap, vtf and choreoobjects have upstream wscripts but are only part of waf's full game configuration
# (SDL2, freetype, openal, ...), not the --tests one, so their sources are compiled here with the same
# flags and DEFINES; each source list is the one in the library's wscript (colorconversion.cpp's stb_dxt.h comes from the pinned thirdparty commit the Dockerfile
# caches under /opt/toolchains/deps). bitmap's source list is upstream's bitmap/wscript.
mkdir -p "$B/mayhem-extra"
extra_lib() { # <name> <sources...>
  local name=$1; shift; local objs=()
  for f in "$@"; do
    local o="$B/mayhem-extra/${name}_$(basename "$f" .cpp).o"
    $CXX $HFLAGS $COV ${EXTRA_FLAGS:-} -c "$f" -o "$o"
    objs+=("$o")
  done
  rm -f "$B/mayhem-extra/lib$name.a"
  ar rcs "$B/mayhem-extra/lib$name.a" "${objs[@]}"
}
# dmxloader allocates elements/attributes from a DMX-context stack allocator with 4-byte alignment
# (dmxloader.cpp DECLARE_DMX_CONTEXT pool), so on 64-bit every attribute construction trips UBSan's
# `alignment` check before any input-dependent code runs; relax only that check, only for dmxloader.
EXTRA_FLAGS="$([ -n "$SAN" ] && echo -fno-sanitize=alignment)" extra_lib dmxloader dmxloader/dmxattribute.cpp dmxloader/dmxelement.cpp dmxloader/dmxloader.cpp \
  dmxloader/dmxloadertext.cpp dmxloader/dmxserializationdictionary.cpp
extra_lib bitmap bitmap/ImageByteSwap.cpp bitmap/colorconversion.cpp bitmap/float_bm.cpp \
  bitmap/float_bm2.cpp bitmap/float_bm3.cpp bitmap/float_bm_bilateral_filter.cpp bitmap/float_cube.cpp \
  bitmap/imageformat.cpp bitmap/psd.cpp bitmap/resample.cpp bitmap/tgaloader.cpp bitmap/tgawriter.cpp \
  bitmap/bitmap.cpp
extra_lib vtf vtf/convert_x360.cpp vtf/vtf.cpp
extra_lib choreoobjects game/shared/choreoactor.cpp game/shared/choreochannel.cpp \
  game/shared/choreoevent.cpp game/shared/choreoscene.cpp game/shared/sceneimage.cpp \
  public/interpolatortypes.cpp  # choreoscene's curve helpers; scene consumers (utils/vcd_sound_check.vpc) add it
LIBS="$B/mayhem-extra/libdmxloader.a $B/mayhem-extra/libchoreoobjects.a $B/mayhem-extra/libvtf.a $B/mayhem-extra/libbitmap.a $LIBS"

$CXX $SAN $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$B/lsan_off.o"
$CXX $SAN $DEBUG_FLAGS -c mayhem/cpufreq_env.cc -o "$B/cpufreq_env.o"
$CC $SAN $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
for h in mayhem/harnesses/fuzz_*.cpp; do
  name=$(basename "$h" .cpp)
  $CXX $HFLAGS -c "$h" -o "$B/$name.o"
  $CXX $SAN $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$B/$name.o" "$B/lsan_off.o" "$B/cpufreq_env.o" $LIBS -o "$OUT/$name"
  $CXX $SAN $DEBUG_FLAGS "$B/$name.o" "$B/standalone_main.o" "$B/lsan_off.o" "$B/cpufreq_env.o" $LIBS -o "$OUT/$name-standalone"
done
echo "built: $(ls "$OUT"/fuzz_* | tr '\n' ' ')"
