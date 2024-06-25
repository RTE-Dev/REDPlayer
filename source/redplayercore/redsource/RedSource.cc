#include "RedSource.h"
#include "RedExtractorFactory.h"
#include "RedFFExtractor.h"
#include "RedLog.h"

REDSOURCE_NS_BEGIN
RedSource::RedSource() : RedSource(0, nullptr) {}

RedSource::RedSource(const int &session_id, NotifyCallback notify_cb)
    : session_id_(session_id) {
  extractor_ = RedExtractorFactory::create(session_id_, ExtractorType::FFMpeg,
                                           notify_cb);
}

RedSource::~RedSource() {
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]RedSource Deconstruct \n",
             __FUNCTION__, __LINE__);
}

void RedSource::close() {
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]RedSource close start! \n",
             __FUNCTION__, __LINE__);
  if (extractor_) {
    extractor_->close();
  }
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]RedSource close end! \n",
             __FUNCTION__, __LINE__);
}

int RedSource::seek(int64_t timestamp, int64_t rel, int seek_flags) {
  if (extractor_) {
    return extractor_->seek(timestamp, rel, seek_flags);
  }
  AV_LOGE_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]extractor_ null! \n",
             __FUNCTION__, __LINE__);
  return -1;
}

int RedSource::open(const std::string &url, FFMpegOpt &opt,
                    std::shared_ptr<MetaData> &metadata) {
  if (extractor_) {
    return extractor_->open(url, opt, metadata);
  }
  AV_LOGE_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]extractor_ null! \n",
             __FUNCTION__, __LINE__);
  return -1;
}

int RedSource::readPacket(AVPacket *pkt) {
  int ret = -1;
  if (extractor_) {
    return extractor_->readPacket(pkt);
  }
  AV_LOGE_ID(SOURCE_LOG_TAG, session_id_, "[%s,%d]extractor_ null! \n",
             __FUNCTION__, __LINE__);
  return ret;
}

void RedSource::setInterrupt() {
  if (extractor_) {
    extractor_->setInterrupt();
  }
}

int RedSource::getPbError() {
  int ret = 0;
  if (extractor_) {
    ret = extractor_->getPbError();
  }
  return ret;
}

int RedSource::getStreamType(int stream_index) {
  int ret = -1;
  if (extractor_) {
    ret = extractor_->getStreamType(stream_index);
  }
  return ret;
}
REDSOURCE_NS_END
