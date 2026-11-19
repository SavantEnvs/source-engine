// Bundled snappy decompressor (tier1/snappy.cpp).
#include "fuzz_common.h"
#include "tier1/snappy.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  char *in = (char *)malloc(size ? size : 1);
  memcpy(in, data, size);
  size_t len = 0;
  bool valid = snappy::IsValidCompressedBuffer(in, size);
  if (snappy::GetUncompressedLength(in, size, &len) && len <= (1u << 22)) {
    char *out = (char *)malloc(len ? len : 1);
    bool ok = snappy::RawUncompress(in, size, out);
    // The validator and the decoder must agree.
    if (ok != valid) abort();
    free(out);
  }
  free(in);
  return 0;
}
