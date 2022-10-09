#ifndef _VM_BIN_H
#define _VM_BIN_H

#include <cstdint>
#include <cstring>

struct VmText {
  uint8_t len;
  uint8_t *buf;
};

static inline uint32_t vmUInt32(uint8_t *&p) {
  uint32_t d;
  d = *(uint32_t *)p;
  p += sizeof(uint32_t);
  return d;
}

static inline int32_t vmInt32(uint8_t *&p) {
  int32_t d;
  d = *(int32_t *)p;
  p += sizeof(int32_t);
  return d;
}

static inline uint16_t vmUInt16(uint8_t *&p) {
  uint16_t d;
  d = *(uint16_t *)p;
  p += sizeof(uint16_t);
  return d;
}

static inline int16_t vmInt16(uint8_t *&p) {
  int16_t d;
  d = *(int16_t *)p;
  p += sizeof(int16_t);
  return d;
}

static inline uint8_t vmUInt8(uint8_t *&p) {
  uint8_t d;
  d = *(uint8_t *)p;
  p += sizeof(uint8_t);
  return d;
}

static inline int8_t vmInt8(uint8_t *&p) {
  int8_t d;
  d = *(int8_t *)p;
  p += sizeof(int8_t);
  return d;
}

static inline float vmFloat(uint8_t *&p) {
  float d;
  d = *(float *)p;
  p += sizeof(float);
  return d;
}

static inline void vmText(VmText &text, uint8_t *&p) {
  text.len = vmUInt8(p);
  text.buf = new uint8_t[text.len];
  memcpy(text.buf, p, text.len);
  p += text.len;
}
#endif // _VM_BIN_H
