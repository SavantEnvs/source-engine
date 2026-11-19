// RIFF/WAV chunk reader (tier2/riff.cpp): InFileRIFF + IterateRIFF over an in-memory file.
#include "fuzz_common.h"
#include "tier2/riff.h"
#include "tier0/memdbgon.h"

class MemReader : public IFileReadBinary {
 public:
  MemReader(const uint8_t *d, size_t n) : m_d(d), m_n(n), m_pos(0) {}
  intp open(const char *) { m_pos = 0; return 1; }
  int read(void *out, int size, intp) {
    if (size <= 0 || m_pos >= m_n) return 0;
    size_t left = m_n - m_pos;
    size_t k = (size_t)size < left ? (size_t)size : left;
    memcpy(out, m_d + m_pos, k);
    m_pos += k;
    return (int)k;
  }
  void close(intp) {}
  // Like fseek on the original stdio reader: seeking past EOF is allowed, reads there return 0.
  void seek(intp, int pos) { if (pos >= 0) m_pos = (size_t)pos; }
  unsigned int tell(intp) { return (unsigned int)m_pos; }
  unsigned int size(intp) { return (unsigned int)m_n; }
 private:
  const uint8_t *m_d;
  size_t m_n, m_pos;
};

// Real WAVs have a handful of chunks; a header can claim a 2 GB RIFF of empty 8-byte chunks, so the
// walk stops after a bounded number of chunks per input (a fixed work bound, not a timer).
static int g_chunks;
static void Walk(IterateRIFF &it, int depth) {
  static char scratch[4096];
  while (it.ChunkAvailable() && g_chunks++ < 4096) {
    unsigned int name = it.ChunkName();
    unsigned int sz = it.ChunkSize();
    if (name == MAKEID('L', 'I', 'S', 'T') && depth < 8) {
      IterateRIFF sub(it);
      Walk(sub, depth + 1);
    } else if (sz <= sizeof(scratch)) {
      it.ChunkRead(scratch);
    } else {
      it.ChunkReadPartial(scratch, sizeof(scratch));
    }
    it.ChunkNext();
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t *copy = (uint8_t *)malloc(size ? size : 1);
  memcpy(copy, data, size);
  g_chunks = 0;
  MemReader io(copy, size);
  InFileRIFF riff("fuzz.wav", io);
  if (riff.IsValid() && riff.RIFFSize() > 0) {
    IterateRIFF walk(riff, riff.RIFFSize());
    Walk(walk, 0);
  }
  free(copy);
  return 0;
}
