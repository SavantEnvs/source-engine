// TGA image loader (bitmap/tgaloader.cpp): TGALoader::GetInfo + TGALoader::LoadRGBA8888.
#include "fuzz_common.h"
#include "bitmap/tgaloader.h"
#include "tier1/utlmemory.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  CUtlBuffer buf(copy, (int)size, CUtlBuffer::READ_ONLY);
  int w = 0, h = 0;
  ImageFormat fmt;
  float gamma;
  if (TGALoader::GetInfo(buf, &w, &h, &fmt, &gamma)) {
    buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
    CUtlMemory<unsigned char> out;
    TGALoader::LoadRGBA8888(buf, out, w, h);
  }
  free(copy);
  return 0;
}
