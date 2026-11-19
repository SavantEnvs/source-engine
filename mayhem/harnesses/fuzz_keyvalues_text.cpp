// KeyValues text parser (VMT/.res/.cfg/scripts): KeyValues::LoadFromBuffer.
// No filesystem is passed, so #include/#base cannot touch the disk.
#include "fuzz_common.h"
#include "tier1/KeyValues.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // LoadFromBuffer(const char*) expects a NUL-terminated string.
  char *text = (char *)malloc(size + 1);
  memcpy(text, data, size);
  text[size] = 0;
  KeyValues *kv = new KeyValues("fuzz");
  if (kv->LoadFromBuffer("fuzz.txt", text)) {
    // Walk and re-serialize what was parsed.
    for (KeyValues *k = kv; k; k = k->GetNextKey()) {
      for (KeyValues *s = k->GetFirstSubKey(); s; s = s->GetNextKey()) {
        (void)s->GetName();
        (void)s->GetString();
        (void)s->GetInt();
        (void)s->GetFloat();
      }
    }
    CUtlBuffer out(0, 0, CUtlBuffer::TEXT_BUFFER);
    kv->RecursiveSaveToFile(out, 0);
  }
  kv->deleteThis();
  free(text);
  return 0;
}
