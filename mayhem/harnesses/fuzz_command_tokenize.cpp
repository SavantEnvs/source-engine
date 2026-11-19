// Console / cfg / server-sent command text: CCommandBuffer::AddText + CCommand::Tokenize.
#include "fuzz_common.h"
#include "tier1/convar.h"
#include "tier1/CommandBuffer.h"
#include "tier0/memdbgon.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  char *text = (char *)malloc(size + 1);
  memcpy(text, data, size);
  text[size] = 0;

  CCommand cmd;
  if (cmd.Tokenize(text)) {
    for (int i = 0; i < cmd.ArgC(); i++) (void)strlen(cmd.Arg(i));
    (void)cmd.ArgS();
    (void)cmd.FindArg("-x");
  }

  CCommandBuffer cb;
  if (cb.AddText(text)) {
    cb.BeginProcessingCommands(1);
    while (cb.DequeueNextCommand()) {
      const CCommand &c = cb.GetCommand();
      for (int i = 0; i < c.ArgC(); i++) (void)strlen(c.Arg(i));
    }
    cb.EndProcessingCommands();
  }
  free(text);
  return 0;
}
