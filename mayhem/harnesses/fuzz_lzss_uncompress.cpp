// LZSS decompressor used for network/file payloads: CLZSS::SafeUncompress.
#include "fuzz_common.h"
#include "tier1/lzss.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Exact-size heap copy so ASan sees any read past the input.
  uint8_t *in = (uint8_t *)malloc(size ? size : 1);
  memcpy(in, data, size);
  CLZSS lzss;
  // GetActualSize reads the 8-byte lzss_header_t; callers only pass buffers holding one.
  if (size >= sizeof(lzss_header_t) && CLZSS::IsCompressed(in)) {
    unsigned int actual = CLZSS::GetActualSize(in);
    unsigned int cap = actual < (1u << 20) ? actual : (1u << 20);
    uint8_t *out = (uint8_t *)malloc(cap ? cap : 1);
    lzss.SafeUncompress(in, (unsigned int)size, out, cap);
    free(out);
  }
  free(in);
  return 0;
}
