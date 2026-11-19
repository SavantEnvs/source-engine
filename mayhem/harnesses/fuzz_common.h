// Shared includes for the source-engine fuzz harnesses (mayhem/ layer only).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/utlbuffer.h"

// The launcher either opens engine.log or disables the debug logger at startup
// (launcher/launcher.cpp: DebugLogger()->Init / ->Disable). Without either, tier0 buffers every
// spewed message across inputs, which would make one input's result depend on earlier ones.
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc; (void)argv;
  DebugLogger()->Disable();
  return 0;
}
