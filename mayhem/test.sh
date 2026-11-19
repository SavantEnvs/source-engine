#!/usr/bin/env bash
#
# mayhem/test.sh — RUN source-engine's OWN unit tests (utils/unittest + unittests/{tier0,tier1,tier2,tier3,
# mathlib}test), already built by mayhem/build.sh with normal flags into build-test/install. exit 0 = pass. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# upstream's runner executes every registered test suite ("Starting test <Suite>....") and reports a
# failed Shipping_Assert/Assert/Error through its spew handler as "UnitTest Assert:"/"UnitTest Error:",
# returning 1. Each suite is one CTRF test: it passes iff it started and no assert/error was spewed
# before the next suite started. Suites that never start (a neutered runner) count as failed.
EXPECTED_SUITES="TSListTestSuite CommandBufferTestSuite UtlStringTestSuite LZSSSafeUncompressTestSuite MathlibTestSuite"
DIR="$SRC/build-test/install"
[ -x "$DIR/unittest" ] || { echo "missing $DIR/unittest — mayhem/build.sh did not build the test suite" >&2; emit_ctrf "source-engine-unittest" 0 1; exit 1; }
LOG=$(mktemp)
( cd "$DIR" && LD_LIBRARY_PATH="$DIR/bin" ./unittest ) >"$LOG" 2>&1
rc=$?
cat "$LOG"
results=$(awk '
  /^Starting test / { if (cur != "") print cur, bad; cur = $3; sub(/\.+$/, "", cur); bad = 0; next }
  /^UnitTest (Assert|Error):/ { bad = 1 }
  END { if (cur != "") print cur, bad }' "$LOG")
rm -f "$LOG"
passed=0; failed=0
for s in $EXPECTED_SUITES; do
  st=$(printf '%s\n' "$results" | awk -v s="$s" '$1 == s { print $2 }')
  if [ "$st" = "0" ]; then passed=$((passed + 1)); echo "PASS $s"; else failed=$((failed + 1)); echo "FAIL $s (${st:-not run})"; fi
done
# Any extra suite upstream adds is counted too.
for s in $(printf '%s\n' "$results" | awk '{ print $1 }'); do
  case " $EXPECTED_SUITES " in *" $s "*) ;; *)
    if printf '%s\n' "$results" | awk -v s="$s" '$1 == s && $2 == 0 { ok = 1 } END { exit !ok }'; then passed=$((passed + 1)); else failed=$((failed + 1)); fi ;;
  esac
done
# The runner's own verdict must agree (it returns 1 on any spewed assert/error).
[ "$rc" -eq 0 ] || { [ "$failed" -gt 0 ] || failed=1; echo "unittest exited $rc"; }
emit_ctrf "source-engine-unittest" "$passed" "$failed"
