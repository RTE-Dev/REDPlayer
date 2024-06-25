#pragma once

#include <stdint.h>

#include <atomic>

#ifndef RBUFFER_SIZE
#define RBUFFER_SIZE 1024 * 64
#endif
class Ringbuffer {
public:
  Ringbuffer(int size = RBUFFER_SIZE);
  ~Ringbuffer();
  void flush();
  uint32_t readdata(uint8_t *buf, uint32_t size);
  uint32_t writedata(uint8_t *buf, uint32_t size);
  uint32_t getreadablesize();
  uint32_t getwriteablesize();
  bool isvaliddata(uint64_t datapos);
  uint32_t
  seekinternal(uint64_t seekpos); // seek & read should be in same thread
private:
  uint8_t *mbuf;
  unsigned int msize;
  unsigned int mreadptr;
  unsigned int mwriteptr;
  uint64_t mlogicalpos;
  std::atomic<uint32_t> mcachesize;
};
