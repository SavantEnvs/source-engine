// Binary compiled choreography scene parser (game/shared/choreoscene.cpp):
// CChoreoScene::RestoreFromBinaryBuffer, the way CSceneEntity::LoadScene (game/server/sceneentity.cpp)
// loads a scene from scenes.image. As there, strings come from a read-only pool (the image's string
// table); FindOrAddString is never called at load time on the server.
#include "fuzz_common.h"
#include "choreoscene.h"
#include "tier0/memdbgon.h"

class FixedStringPool : public IChoreoStringPool {
 public:
  short FindOrAddString(const char *) { return -1; }
  bool GetString(short id, char *buff, int buffSize) {
    static const char *const kStrings[] = {"actor", "channel", "event", "scenes/fuzz.vcd",
                                           "Expression", "phoneme", "!target1", "1.0"};
    if (id < 0 || id >= (short)(sizeof(kStrings) / sizeof(kStrings[0]))) {
      V_strncpy(buff, "", buffSize);
      return false;
    }
    V_strncpy(buff, kStrings[id], buffSize);
    return true;
  }
};
static FixedStringPool g_Pool;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  CUtlBuffer buf(copy, (int)size, CUtlBuffer::READ_ONLY);
  CChoreoScene *scene = new CChoreoScene(NULL);
  scene->RestoreFromBinaryBuffer(buf, "scenes/fuzz.vcd", &g_Pool);
  delete scene;
  free(copy);
  return 0;
}
