// Binary KeyValues (network / save data): KeyValues::ReadAsBinary and KVPacker::ReadAsBinary.
#include "fuzz_common.h"
#include "tier1/KeyValues.h"
#include "tier1/kvpacker.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 1) return 0;
  // First byte selects the reader; the rest is the serialized blob.
  uint8_t sel = data[0];
  data++; size--;
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  CUtlBuffer buf(copy, (int)size, CUtlBuffer::READ_ONLY);
  KeyValues *kv = new KeyValues("fuzz");
  bool ok;
  if (sel & 1) {
    KVPacker packer;
    ok = packer.ReadAsBinary(kv, buf);
  } else {
    ok = kv->ReadAsBinary(buf);
  }
  if (ok) {
    CUtlBuffer out;
    kv->WriteAsBinary(out);
  }
  kv->deleteThis();
  free(copy);
  return 0;
}
