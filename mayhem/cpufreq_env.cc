// Environment mock for the fuzz and standalone binaries (libc wrapper, no target code replaced).
//
// tier0 measures the CPU clock in a static initializer (CClockSpeedInit, tier0/fasttimer.cpp ->
// CalculateCPUFreq, tier0/cpu_posix.cpp). It first reads
// /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq; only when that file is missing (containers,
// the Mayhem workers) does it fall back to a TSC calibration loop of up to 600 x usleep(5000). That
// fallback costs seconds on every process start, depends on the host's timing, and trips UBSan
// (signed overflow in diff(), cpu_posix.cpp:51) before any input is read. This wrapper supplies that
// one sysfs file with a fixed 2 GHz value (the frequency cpu_posix.cpp hard-codes for ARM), so every
// host takes upstream's normal sysfs path. Every other fopen goes to libc.
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static const char kCpuFreqPath[] = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";

static FILE *mock_or_real(const char *sym, const char *path, const char *mode) {
  if (path && strcmp(path, kCpuFreqPath) == 0) {
    static const char kKHz[] = "2000000\n";
    return fmemopen((void *)kKHz, sizeof(kKHz) - 1, "r");
  }
  typedef FILE *(*fopen_t)(const char *, const char *);
  fopen_t real = (fopen_t)dlsym(RTLD_NEXT, sym);
  return real(path, mode);
}

extern "C" FILE *fopen(const char *path, const char *mode) { return mock_or_real("fopen", path, mode); }
extern "C" FILE *fopen64(const char *path, const char *mode) { return mock_or_real("fopen64", path, mode); }
