// JSON -> KeyValues parser: KeyValuesJSONParser::ParseFile.
#include "fuzz_common.h"
#include "tier1/KeyValues.h"
#include "tier1/keyvaluesjson.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > (1u << 20)) return 0;
  char *text = (char *)malloc(size ? size : 1);
  memcpy(text, data, size);
  KeyValuesJSONParser parser(text, (int)size);
  KeyValues *kv = parser.ParseFile();
  if (kv) kv->deleteThis();
  free(text);
  return 0;
}
