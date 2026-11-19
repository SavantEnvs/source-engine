// VTF texture loader (vtf/vtf.cpp): CVTFTexture::Unserialize, the way materialsystem loads a .vtf
// read from a map or mod (CreateVTFTexture -> Unserialize(buf) -> DestroyVTFTexture).
#include "fuzz_common.h"
#include "vtf/vtf.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  CUtlBuffer buf(copy, (int)size, CUtlBuffer::READ_ONLY);
  IVTFTexture *tex = CreateVTFTexture();
  tex->Unserialize(buf);
  DestroyVTFTexture(tex);
  free(copy);
  return 0;
}
