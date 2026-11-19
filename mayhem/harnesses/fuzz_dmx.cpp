// DMX deserializer: UnserializeDMX(CUtlBuffer&, ...) from dmxloader. Callers choose binary or
// keyvalues2 text mode (the file overload's bTextMode, i.e. a TEXT_BUFFER CUtlBuffer); the first input
// byte makes that choice here and the rest is the file.
#include "fuzz_common.h"
#include "dmxloader/dmxloader.h"
#include "dmxloader/dmxelement.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 1) return 0;
  bool text = data[0] & 1;
  data++; size--;
  // The header is read as a NUL-terminated string, as from a filesystem text read.
  char *copy = (char *)malloc(size + 1);
  memcpy(copy, data, size);
  copy[size] = 0;
  DECLARE_DMX_CONTEXT();
  CUtlBuffer buf(copy, (int)size + 1, CUtlBuffer::READ_ONLY | (text ? CUtlBuffer::TEXT_BUFFER : 0));
  CDmxElement *root = NULL;
  if (UnserializeDMX(buf, &root, "fuzz.dmx") && root) CleanupDMX(root);
  free(copy);
  return 0;
}
