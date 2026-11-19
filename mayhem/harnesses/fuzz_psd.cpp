// Photoshop PSD reader (bitmap/psd.cpp): IsPSDFile, PSDGetInfo, image resources, PSDReadFileRGBA8888.
#include "fuzz_common.h"
#include "bitmap/psd.h"
#include "bitmap/bitmap.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  CUtlBuffer buf(copy, (int)size, CUtlBuffer::READ_ONLY);
  if (IsPSDFile(buf)) {
    int w = 0, h = 0;
    ImageFormat fmt;
    float gamma;
    buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
    if (PSDGetInfo(buf, &w, &h, &fmt, &gamma)) {
      buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
      PSDImageResources res = PSDGetImageResources(buf);
      PSDResFileInfo info(res.FindElement(PSDImageResources::eResFileInfo));
      (void)info.FindElement(PSDResFileInfo::eTitle);
      buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
      Bitmap_t bm;
      PSDReadFileRGBA8888(buf, bm);
    }
  }
  free(copy);
  return 0;
}
