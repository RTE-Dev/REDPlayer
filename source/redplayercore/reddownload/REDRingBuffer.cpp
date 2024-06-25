#include "REDRingBuffer.h"

#include <cstdlib>
#include <cstring>

#include "RedLog.h"

#define min(a, b) (a) < (b) ? (a) : (b)
#define LOG_TAG "RedRingBuffer"

using namespace std;

Ringbuffer::Ringbuffer(int size) {
  msize = size;
  mreadptr = 0;
  mwriteptr = 0;
  mlogicalpos = 0;
  mcachesize = 0;
  mbuf = reinterpret_cast<uint8_t *>(malloc(size));
  if (mbuf == nullptr)
    AV_LOGW(LOG_TAG, "malloc size failed");
  else
    memset(mbuf, 0, size);
}

Ringbuffer::~Ringbuffer() {
  if (mbuf != nullptr)
    free(mbuf);
  mbuf = nullptr;
}

void Ringbuffer::flush() {
  mreadptr = 0;
  mwriteptr = 0;
  mlogicalpos = 0;
  mcachesize = 0;
}

uint32_t Ringbuffer::readdata(uint8_t *buf, uint32_t readsize) {
  if (mcachesize.load() <= 0) {
    return 0;
  }
  AV_LOGW(LOG_TAG, "ringbuffer read %d, %d, %d\n", mreadptr, mwriteptr, msize);
  readsize = min(readsize, mcachesize.load());
  if (readsize + mreadptr > msize) {
    unsigned int chunk = msize - mreadptr;
    memcpy(buf, mbuf + mreadptr, chunk);
    memcpy(buf + chunk, mbuf, readsize - chunk);
    mreadptr = readsize - chunk;
  } else {
    memcpy(buf, mbuf + mreadptr, readsize);
    mreadptr += readsize;
  }

  if (mreadptr == msize) {
    mreadptr = 0;
  }

  mcachesize -= readsize;
  mlogicalpos += readsize;
  return readsize;
}

uint32_t Ringbuffer::writedata(uint8_t *buf, uint32_t writesize) {
  if (mcachesize.load() == msize)
    return 0;
  writesize = min(writesize, msize - mcachesize.load());

  if (writesize + mwriteptr > msize) {
    unsigned int chunk = msize - mwriteptr;
    memcpy(mbuf + mwriteptr, buf, chunk);
    memcpy(mbuf, buf + chunk, writesize - chunk);
    mwriteptr = writesize - chunk;
  } else {
    memcpy(mbuf + mwriteptr, buf, writesize);
    mwriteptr += writesize;
  }

  if (mwriteptr == msize) {
    mwriteptr = 0;
  }

  mcachesize += writesize;
  AV_LOGW(LOG_TAG, "ringbuffer write %d, %d, %d\n", mreadptr, mwriteptr, msize);
  return writesize;
}

uint32_t Ringbuffer::getreadablesize() { return mcachesize.load(); }

uint32_t Ringbuffer::getwriteablesize() { return msize - mcachesize.load(); }

bool Ringbuffer::isvaliddata(uint64_t datapos) {
  return datapos >= mlogicalpos && datapos < mlogicalpos + mcachesize.load();
}

uint32_t Ringbuffer::seekinternal(uint64_t seekpos) {
  if (!isvaliddata(seekpos))
    return 0;
  int skipsize = static_cast<int>(seekpos - mlogicalpos);
  if (skipsize == 0) {
    return (uint32_t)mlogicalpos;
  }
  if (skipsize + mreadptr > msize) {
    unsigned int chunk = msize - mreadptr;
    mreadptr = skipsize - chunk;
  } else {
    mreadptr += skipsize;
  }

  if (mreadptr == msize) {
    mreadptr = 0;
  }

  mcachesize -= skipsize;
  mlogicalpos += skipsize;
  return (uint32_t)mlogicalpos;
}
