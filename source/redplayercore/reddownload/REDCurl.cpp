#include "REDCurl.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <unistd.h>
#ifdef __APPLE__
#include <resolv.h>
#endif
#include <string>

#include "NetworkQuality.h"
#include "REDURLParser.h"
#include "RedBase.h"
#include "RedLog.h"
#include "dnscache/REDDnsCache.h"
#include "utility/Utility.h"
#define MAX_RETRY 5
#define LOG_TAG "RedCurl"
using namespace std;

CURLSH *RedCurl::share_handle = nullptr;
std::mutex RedCurl::sharehandlemutex;
RedCurl *RedCurl::se = nullptr;
pthread_rwlock_t RedCurl::m_dnsLock;
pthread_rwlock_t RedCurl::m_sslLock;

RedCurl::RedCurl(int dummy) : RedDownloadBase(Source::CDN, 0) {
  mBDummy = true;
  curl_global_init(CURL_GLOBAL_ALL);
  pthread_rwlock_init(&m_dnsLock, nullptr);
  pthread_rwlock_init(&m_sslLock, nullptr);
  std::lock_guard<std::mutex> lock(sharehandlemutex);
  share_handle = curl_share_init();
  curl_share_setopt(share_handle, CURLSHOPT_LOCKFUNC, curlLock);
  curl_share_setopt(share_handle, CURLSHOPT_UNLOCKFUNC, curlUnlock);
}

void RedCurl::curlLock(CURL *handle, curl_lock_data data,
                       curl_lock_access laccess, void *useptr) {
  if (data == CURL_LOCK_DATA_DNS) {
    if (laccess == CURL_LOCK_ACCESS_SHARED) {
      pthread_rwlock_rdlock(&m_dnsLock);
    } else if (laccess == CURL_LOCK_ACCESS_SINGLE) {
      pthread_rwlock_wrlock(&m_dnsLock);
    }
  } else if (data == CURL_LOCK_DATA_SSL_SESSION) {
    if (laccess == CURL_LOCK_ACCESS_SHARED) {
      pthread_rwlock_rdlock(&m_sslLock);
    } else if (laccess == CURL_LOCK_ACCESS_SINGLE) {
      pthread_rwlock_wrlock(&m_sslLock);
    }
  }
}

void RedCurl::curlUnlock(CURL *handle, curl_lock_data data, void *useptr) {
  if (data == CURL_LOCK_DATA_DNS) {
    pthread_rwlock_unlock(&m_dnsLock);
  } else if (data == CURL_LOCK_DATA_SSL_SESSION) {
    pthread_rwlock_unlock(&m_sslLock);
  }
}

int RedCurl::debugfunc(CURL *handle, curl_infotype type, char *data,
                       size_t size, void *userptr) {
  const char *text = nullptr;
  (void)handle;
  (void)userptr;

  switch (type) {
  case CURLINFO_TEXT:
    text = "== Info:";
    break;
  case CURLINFO_HEADER_OUT:
    text = "=> Send header\n";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data\n";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data\n";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header\n";
    break;
  default:
    return 0;
  }
  if (text && data) {
    (void)memset(m_curldebuginfo, 0, MAX_CURL_DEBUG_LEN);
    (void)memcpy(m_curldebuginfo, data,
                 min(size, static_cast<size_t>(MAX_CURL_DEBUG_LEN - 1)));
    AV_LOGI(LOG_TAG, "curl debug(%zu): %s%s", size, text, m_curldebuginfo);
  }
  return 0;
}

int RedCurl::sockopt_callback(void *clientp, curl_socket_t curlfd,
                              curlsocktype purpose) {
  if (purpose == CURLSOCKTYPE_IPCXN) {
    if (clientp == nullptr)
      return CURL_SOCKOPT_OK;
    RedCurl *thiz = reinterpret_cast<RedCurl *>(clientp);
    if (thiz && thiz->bpause.load())
      return CURL_SOCKOPT_OK;

    while (thiz && thiz->mdownpara && thiz->mdownpara->mopt &&
           thiz->mdownpara->mopt->tcp_buffer > 0 && thiz->mdownpara->mdatacb) {
      int actualBufferSize = 0;
      socklen_t actualBufferSizeLength = sizeof(actualBufferSize);
      if (getsockopt(curlfd, SOL_SOCKET, SO_RCVBUF, &actualBufferSize,
                     &actualBufferSizeLength) != 0)
        break;
      AV_LOGI(LOG_TAG,
              "RedCurl %p sockopt_callback before set buffer size: %d "
              "bytes\n",
              thiz, actualBufferSize);
      thiz->mdownpara->mdatacb->DownloadCallBack(
          RED_CTRL_WILL_TCP_OPEN, &actualBufferSize, nullptr, 0, 0);
      thiz->mtcpStartTs = getTimestampMs();
      if (setsockopt(curlfd, SOL_SOCKET, SO_RCVBUF,
                     &thiz->mdownpara->mopt->tcp_buffer,
                     sizeof(thiz->mdownpara->mopt->tcp_buffer)) != 0)
        break;
      if (getsockopt(curlfd, SOL_SOCKET, SO_RCVBUF, &actualBufferSize,
                     &actualBufferSizeLength) != 0)
        break;
      AV_LOGI(LOG_TAG,
              "RedCurl %p sockopt_callback after set buffer size: %d bytes\n",
              thiz, actualBufferSize);
      break;
    }
  }
  return CURL_SOCKOPT_OK;
}

curl_socket_t RedCurl::opensocket_callback(void *clientp, curlsocktype purpose,
                                           struct curl_sockaddr *address) {
  if (purpose == CURLSOCKTYPE_IPCXN) {
    if (clientp != nullptr) {
      RedCurl *thiz = reinterpret_cast<RedCurl *>(clientp);
      if (thiz)
        thiz->dnsCallback(address, DnsStatus::Success);
    }
    return socket(address->family, address->socktype, address->protocol);
  }
  return CURL_SOCKET_BAD;
}

RedCurl::RedCurl(Source source, int range_size) : RedDownloadBase(source, 0) {
  std::call_once(globalCurlInitOnceFlag, [&] { se = new RedCurl(0); });
  multi_handle = curl_multi_init();
  mcurl = nullptr;
  bpause.store(false);
  m_pause_timeout.store(false);
  max_retry = MAX_RETRY;
  mfilelength = 0;
  brunning = false;
  mdownpara = nullptr;
  mserial = 0;
  mnotitystate = 0;
  m_range_size = range_size;
  m_callback_dns = RedDownloadConfig::getinstance()->get_config_value(
                       CALLBACK_DNS_INFO_KEY) > 0;
}

RedCurl::~RedCurl() {
  if (mBDummy) {
    if (share_handle) {
      curl_share_cleanup(share_handle);
      share_handle = nullptr;
    }
    curl_global_cleanup();
    pthread_rwlock_destroy(&m_dnsLock);
    pthread_rwlock_destroy(&m_sslLock);
    return;
  }

  destroyCurl();
  if (multi_handle != nullptr)
    curl_multi_cleanup(multi_handle);
  multi_handle = nullptr;
  AV_LOGI(LOG_TAG, "RedCurl %p %s\n", this, __FUNCTION__);
}

bool RedCurl::destroyCurl() {
  // AV_LOGI(LOG_TAG, "RedCurl %p %s\n",this, __FUNCTION__);
  pause(true);
  // resetDownloadStatus();
  m_curlcode = CURLE_OK;
  if (mhost != nullptr) {
    curl_slist_free_all(mhost);
    mhost = nullptr;
  }
  if (mheaderList != nullptr) {
    curl_slist_free_all(mheaderList);
    mheaderList = nullptr;
  }
  if (multi_handle != nullptr && mcurl != nullptr) {
    curl_multi_remove_handle(multi_handle, mcurl);
    curl_easy_cleanup(mcurl);
  }
  mcurl = nullptr;
  // AV_LOGI(LOG_TAG, "RedCurl %p %s end\n", this, __FUNCTION__);
  return true;
}
void RedCurl::readdns(RedDownLoadPara *downpara) {
  mdownpara = downpara;
  max_retry = 1; // read dns no need for retry
  m_is_read_dns = true;
  updateCurl();
  getheadinfo();
  return;
}

bool RedCurl::prepare() {
  mretrycount = 0;
  bdescurl = false;
  if (mdownpara != nullptr && mdownpara->mopt != nullptr) {
    bdescurl = mdownpara->mopt->PreDownLoadSize > 0;
  }
  if (mfilelength == 0) {
    if (mcurl == nullptr) {
      if (!initCurl())
        return false;
    }

    if (bpause.load()) {
      if (mcurl) {
        curl_easy_cleanup(mcurl);
        mcurl = nullptr;
      }
      return false;
    }
    int64_t starttime = CurrentTimeUs();
    if (mdownpara != nullptr && mdownpara->mdatacb != nullptr &&
        mdownpara->mopt != nullptr) {
      mdownpara->mdatacb->DownloadCallBack(RED_EVENT_WILL_HTTP_OPEN, nullptr,
                                           nullptr, 0, 0);
      mhttpStartTs = getTimestampMs();
      if (mdownpara->mopt->tcp_buffer <= 0) {
        mdownpara->mdatacb->DownloadCallBack(RED_CTRL_WILL_TCP_OPEN, nullptr,
                                             nullptr, 0, 0);
        mtcpStartTs = mhttpStartTs;
      }
    }
    AV_LOGI(LOG_TAG, "RedCurl %p DownloadCallBack cost:%" PRId64 "\n", this,
            CurrentTimeUs() - starttime);
  }

  updateCurl();
  if (mcurl == nullptr)
    return false;

  return true;
}

bool RedCurl::initCurl() {
  if ((mdownpara == nullptr) || (mdownpara->mopt == nullptr)) {
    AV_LOGW(LOG_TAG, "RedCurl %p null download para\n", this);
    return false;
  }
  DownLoadOpt opt = *mdownpara->mopt;
  m_islive = opt.islive;
  if (mcurl != nullptr)
    curl_easy_cleanup(mcurl);
  mcurl = curl_easy_init();
  if (!mcurl)
    return false;
  if (bpause.load()) {
    curl_easy_cleanup(mcurl);
    mcurl = nullptr;
    return false;
  }
  // CURLcode ret = CURLE_OK;
  {
    std::lock_guard<std::mutex> lock(sharehandlemutex);
    if (share_handle == nullptr) {
      share_handle = curl_share_init();
      curl_share_setopt(share_handle, CURLSHOPT_LOCKFUNC, curlLock);
      curl_share_setopt(share_handle, CURLSHOPT_UNLOCKFUNC, curlUnlock);
    }
    if (RedDownloadConfig::getinstance()->get_config_value(SHAREDNS_KEY) == 1 ||
        m_islive) {
      if (m_islive && RedDownloadConfig::getinstance()->get_config_value(
                          SHAREDNS_LIVE_KEY) > 0) {
        curl_share_setopt(share_handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_DNS);
        // AV_LOGI(LOG_TAG, "RedCurl %p %s set UNSHAREDNS live ", this,
        // __FUNCTION__);
      } else {
        curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        // AV_LOGI(LOG_TAG, "RedCurl %p %s set SHAREDNS ", this,
        // __FUNCTION__);
      }
    } else {
      curl_share_setopt(share_handle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_DNS);
    }
    if (opt.use_https) {
      curl_share_setopt(share_handle, CURLSHOPT_SHARE,
                        CURL_LOCK_DATA_SSL_SESSION);
    } else {
      curl_share_setopt(share_handle, CURLSHOPT_UNSHARE,
                        CURL_LOCK_DATA_SSL_SESSION);
    }
    curl_easy_setopt(mcurl, CURLOPT_SHARE, share_handle);
  }
  curl_easy_setopt(mcurl, CURLOPT_DNS_CACHE_TIMEOUT,
                   opt.dns_cache_timeout / 1000000);
  if (!opt.http_proxy.empty()) {
    curl_easy_setopt(mcurl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
    curl_easy_setopt(mcurl, CURLOPT_PROXY, opt.http_proxy.c_str());
    AV_LOGI(LOG_TAG, "RedCurl %p %s use http proxy: %s", this, __FUNCTION__,
            opt.http_proxy.c_str());
  }
  if (RedDownloadConfig::getinstance()->get_config_value(
          OPEN_CURL_DEBUG_INFO_KEY) > 0) {
    curl_easy_setopt(mcurl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(mcurl, CURLOPT_DEBUGFUNCTION, &RedCurl::debugfunc);
  } else {
    curl_easy_setopt(mcurl, CURLOPT_VERBOSE, 0L);
  }
  curl_easy_setopt(mcurl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(mcurl, CURLOPT_MAXREDIRS, 4L);
  curl_easy_setopt(mcurl, CURLOPT_HEADERFUNCTION, &RedCurl::headerfunc);
  curl_easy_setopt(mcurl, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(mcurl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(mcurl, CURLOPT_SSL_VERIFYHOST, 0L);
  if (RedDownloadConfig::getinstance()->get_config_value(IPRESOLVE_KEY) == 1) {
    curl_easy_setopt(mcurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    AV_LOGI(LOG_TAG, "RedCurl %p %s CURLOPT_IPRESOLVE: CURL_IPRESOLVE_V4", this,
            __FUNCTION__);
  } else if (RedDownloadConfig::getinstance()->get_config_value(
                 IPRESOLVE_KEY) == 2) {
    curl_easy_setopt(mcurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
    AV_LOGI(LOG_TAG, "RedCurl %p %s CURLOPT_IPRESOLVE: CURL_IPRESOLVE_V6", this,
            __FUNCTION__);
  } else {
    curl_easy_setopt(mcurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
    AV_LOGI(LOG_TAG, "RedCurl %p %s CURLOPT_IPRESOLVE: CURL_IPRESOLVE_WHATEVER",
            this, __FUNCTION__);
  }
  if (opt.showprogress) {
    curl_easy_setopt(mcurl, CURLOPT_NOPROGRESS, 0L);
  }
  curl_easy_setopt(mcurl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(mcurl, CURLOPT_CONNECTTIMEOUT_MS,
                   opt.connect_timeout / 1000);
  curl_easy_setopt(mcurl, CURLOPT_LOW_SPEED_LIMIT, opt.low_speed_limit);
  curl_easy_setopt(mcurl, CURLOPT_LOW_SPEED_TIME, opt.low_speed_time);
  if (!opt.useragent.empty())
    curl_easy_setopt(mcurl, CURLOPT_USERAGENT, opt.useragent.c_str());
  if (mheaderList != nullptr) {
    curl_slist_free_all(mheaderList);
    mheaderList = nullptr;
  }
  for (string &header : opt.headers) {
    if (!header.empty()) {
      mheaderList = curl_slist_append(mheaderList, header.c_str());
    }
  }
  curl_easy_setopt(mcurl, CURLOPT_HTTPHEADER, mheaderList);
  if (!opt.referer.empty())
    curl_easy_setopt(mcurl, CURLOPT_REFERER, opt.referer.c_str());
  if (opt.max_retry > 0) {
    max_retry = opt.max_retry;
  }
  if (opt.tcp_buffer > 0) {
    curl_easy_setopt(mcurl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
    curl_easy_setopt(mcurl, CURLOPT_SOCKOPTDATA, this);
    AV_LOGI(LOG_TAG, "RedCurl %p %s tcp_buffer:%d bytes\n", this, __FUNCTION__,
            opt.tcp_buffer);
  }
  if (m_callback_dns) {
    curl_easy_setopt(mcurl, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
    curl_easy_setopt(mcurl, CURLOPT_OPENSOCKETDATA, this);
  }
  return true;
}

int RedCurl::filldata(RedDownLoadPara *downpara, size_t size) {
  if (m_range_size > 0) {
    return filldata_by_range_size(downpara, size);
  }
  mdownpara = downpara;
  if (mdownpara && mdownpara->mopt->islive)
    mretrycount = 0;
  if (mcurl == nullptr) {
    if (!prepare())
      return ERROR(EAGAIN);
  }
  int rescurl = 0;

  if (!PerformCurl(rescurl) && rescurl == 0) {
    rescurl = EOF;
  }

  if ((mfilelength == 0) && (rescurl != 0) && (rescurl != CURLE_WRITE_ERROR) &&
      !mdownpara->mopt->islive) {
    HttpCallBack(rescurl);
  }
  if (rescurl > 0) {
    rescurl = CURLERROR(rescurl);
  }
  if (rescurl < 0) {
    ErrorData data = rescurl;
    eventCallback(RED_EVENT_ERROR, &data);
  }
  return rescurl;
}

int RedCurl::filldata_by_range_size(RedDownLoadPara *downpara, size_t size) {
  mdownpara = downpara;
  if (mdownpara && mdownpara->mopt->islive)
    mretrycount = 0;
  if (mcurl == nullptr) {
    if (!prepare())
      return ERROR(EAGAIN);
  }
  int rescurl = 0;

  PerformCurl(rescurl);

  if ((mfilelength == 0) && (rescurl != 0) && (rescurl != CURLE_WRITE_ERROR) &&
      !mdownpara->mopt->islive) {
    HttpCallBack(rescurl);
  }
  if (rescurl > 0) {
    rescurl = CURLERROR(rescurl);
  }
  if (rescurl < 0) {
    ErrorData data = rescurl;
    eventCallback(RED_EVENT_ERROR, &data);
  }
  return rescurl;
}

void RedCurl::updateCurl() {
  if (mcurl == nullptr) {
    if (!initCurl()) {
      AV_LOGI(LOG_TAG, "RedCurl %p update curl init failed \n", this);
      return;
    }
  }
  m_starttime = CurrentTimeUs();
  curl_multi_remove_handle(multi_handle, mcurl);
  mserial = static_cast<int>(mdownpara->serial);
  std::string range("");

  if (mdownpara->mopt->PreDownLoadSize == 0 && m_range_size > 0) {
    mdownpara->range_end = mdownpara->range_start + m_range_size - 1;
  }

  if (mdownpara->range_end == 0) {
    range += (std::to_string(mdownpara->range_start) + "-");
    AV_LOGI(LOG_TAG, "RedCurl %p curl-handler %s, %p range %s, %" PRIu64 "--\n",
            this, __FUNCTION__, mdownpara, range.c_str(),
            mdownpara->range_start);
  } else {
    range += (std::to_string(mdownpara->range_start) + "-" +
              std::to_string(mdownpara->range_end));
    AV_LOGI(LOG_TAG, "RedCurl %p %s, %p range %s, %" PRIu64 "--%" PRIu64 "\n",
            this, __FUNCTION__, mdownpara, range.c_str(),
            mdownpara->range_start, mdownpara->range_end);
  }
  curl_easy_setopt(mcurl, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(mcurl, CURLOPT_HEADER, 0L);
  curl_easy_setopt(mcurl, CURLOPT_RANGE, range.c_str());
  curl_easy_setopt(mcurl, CURLOPT_WRITEFUNCTION, &RedCurl::writefunc);
  curl_easy_setopt(mcurl, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(mcurl, CURLOPT_HEADERFUNCTION, &RedCurl::headerfunc);
  curl_easy_setopt(mcurl, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(mcurl, CURLOPT_URL, mdownpara->url.c_str());

  if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) > 0 &&
      !m_islive) {
    UrlParser up(mdownpara->url);
    string hostname = up.getdomain();
    string prototol = up.getprotocol();
    string port = "80";
    if (strcmp(prototol.c_str(), "https") == 0) {
      port = "443";
    }
    string hostinfo = hostname + ":" + port + ":";
    mhostname = hostname;
    string ipaddr = REDDnsCache::getinstance()->GetIpAddr(mhostname);
    mcuripaddr = ipaddr;
    if (!ipaddr.empty()) {
      mdownloadStatus->errorcode = 0;
      hostinfo += ipaddr;
      if (mhost != nullptr) {
        curl_slist_free_all(mhost);
        mhost = nullptr;
      }
      mhost = curl_slist_append(nullptr, hostinfo.c_str());
      curl_easy_setopt(mcurl, CURLOPT_RESOLVE, mhost);
    }
    AV_LOGI(LOG_TAG, "RedCurl %p %s getipaddr %s, ip %s\n", this, __FUNCTION__,
            hostinfo.c_str(), ipaddr.c_str());
  }

  curl_multi_add_handle(multi_handle, mcurl);
  if (mfilelength > 0)
    mdownpara->filesize = mfilelength;

  if (m_callback_dns) {
    dnsCallback(nullptr, DnsStatus::Waiting);
  }
}

void RedCurl::HttpCallBack(int err) {
  if (bpause.load()) {
    AV_LOGI(LOG_TAG, "RedCurl %p %s, pause, return\n", this, __FUNCTION__);
    return;
  }
  if (mdownpara->mopt != nullptr && mdownpara->mdatacb != nullptr) {
    int64_t httpCode = 0;
    curl_easy_getinfo(mcurl, CURLINFO_RESPONSE_CODE, &httpCode);
    mdownloadStatus->httpcode = static_cast<int>(httpCode);
    RedHttpEvent *mdata = new RedHttpEvent;
    mdata->error = err;
    mdata->http_code = static_cast<int>(httpCode);
    mdata->filesize = mfilelength;
    mdata->url = mdownpara->url;
    mdata->offset = mdownpara->range_start;
    mdata->http_rtt = mhttpStartTs == 0
                          ? 0
                          : static_cast<int>(getTimestampMs() - mhttpStartTs);
    mdata->source = msource;
    mdownpara->mdatacb->DownloadCallBack(RED_EVENT_DID_HTTP_OPEN,
                                         reinterpret_cast<void *>(mdata),
                                         nullptr, 0, 0);
    delete mdata;
  }
}

void RedCurl::tcpCallback(int errcode) {
  if (bpause.load()) {
    AV_LOGI(LOG_TAG, "RedCurl %p %s, pause, return\n", this, __FUNCTION__);
    return;
  }
  if (mdownpara->mopt != nullptr && mdownpara->mdatacb != nullptr) {
    RedTcpIOControl *tcpdata = new RedTcpIOControl;
    tcpdata->ip = nullptr;
    tcpdata->port = 0;
    tcpdata->fd = 0;
    tcpdata->error = errcode;
    tcpdata->url = mdownpara->url;
    tcpdata->tcp_rtt =
        mtcpStartTs == 0 ? 0 : static_cast<int>(getTimestampMs() - mtcpStartTs);
    tcpdata->source = msource;
    curl_easy_getinfo(mcurl, CURLINFO_PRIMARY_IP, &(tcpdata->ip));
    AV_LOGW(LOG_TAG, "RedCurl %p get head info tcp %s\n", this, tcpdata->ip);
    if (tcpdata->ip != nullptr) {
      mcuripaddr = tcpdata->ip;
      if (strchr(tcpdata->ip, ':') != NULL) {
        tcpdata->family = AF_INET6;
      } else {
        tcpdata->family = AF_INET;
      }
    }
    mdownpara->mdatacb->DownloadCallBack(RED_CTRL_DID_TCP_OPEN,
                                         reinterpret_cast<void *>(tcpdata),
                                         nullptr, 0, 0);
    delete tcpdata;
  }
}

bool RedCurl::PerformCurl(int &curlres) {
  int still_running = 0;
  curl_multi_perform(multi_handle, &still_running);
  int numfds;
  int err = 0;
  curl_multi_wait(multi_handle, NULL, 0, 100, &numfds);
  if (bpause.load() && m_pause_timeout.load()) {
    {
      std::unique_lock<std::mutex> locker(m_mutexCurl);
      brunning = false;
      m_condition.notify_one();
    }
    AV_LOGI(LOG_TAG, "RedCurl %p %s pause time out exit, ret %d\n", this,
            __FUNCTION__, err);
    curlres = ERROR_TCP_CONNECT_TIMEOUT;
    goto failed;
  }
  if ((mdownpara->filesize > 0 && mfilelength == 0) || mnotitystate == 2) {
    mfilelength = mdownpara->filesize;
    double namelookuptime = 0;
    double tcpconnectime = 0;
    curl_easy_getinfo(mcurl, CURLINFO_NAMELOOKUP_TIME, &namelookuptime);
    curl_easy_getinfo(mcurl, CURLINFO_CONNECT_TIME, &tcpconnectime);
    AV_LOGI(LOG_TAG,
            "RedCurl %p %s, get file size %" PRIu64
            ", name lookup time:%.2f, tcp "
            "connect time:%.2f\n",
            this, __FUNCTION__, mfilelength, namelookuptime, tcpconnectime);
    if (mdownpara->mopt && !mdownpara->mopt->islive)
      HttpCallBack(err);
    mnotitystate = 0;
  }

  if (!still_running) {
    m_endtime = CurrentTimeUs();
    int msgs;
    CURLcode CURLResult = CURLE_OK;
    CURLMsg *msg;
    double downloadsize = 0;
    while ((msg = curl_multi_info_read(multi_handle, &msgs))) {
      if (msg->msg == CURLMSG_DONE) {
        if ((msg->data.result == CURLE_OK) || mdownpara->preload_finished) {
          curl_easy_getinfo(msg->easy_handle, CURLINFO_SIZE_DOWNLOAD,
                            &downloadsize);
          AV_LOGI(LOG_TAG, "RedCurl %p %s, %s, download size %.2f\n", this,
                  __FUNCTION__,
                  msg->data.result == CURLE_OK ? "CURLE_OK"
                                               : "preload finished",
                  downloadsize);
        }
        CURLResult = msg->data.result;
        int64_t httpCode = 0;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &httpCode);

        mdownloadStatus->httpcode = static_cast<int>(httpCode);
        int httperr = gethttperror(static_cast<int>(httpCode));
        AV_LOGW(LOG_TAG,
                "RedCurl %p curl_multi_info_read result %s(%d), "
                "retrycount %d\n",
                this, curl_easy_strerror(msg->data.result), msg->data.result,
                mretrycount);

        if (RedDownloadConfig::getinstance()->get_config_value(
                IP_DOWNGRADE_KEY) > 0) {
          if (msg->data.result == CURLE_COULDNT_CONNECT &&
              ((RedDownloadConfig::getinstance()->get_config_value(
                    IPRESOLVE_KEY) == 2) ||
               RedDownloadConfig::getinstance()->get_config_value(
                   IPRESOLVE_KEY) == 0)) {
            char *primaryIP = nullptr;
            curl_easy_getinfo(mcurl, CURLINFO_PRIMARY_IP, &primaryIP);
            AV_LOGW(LOG_TAG, "RedCurl %p connect tcp %s error.\n", this,
                    primaryIP);
            curl_easy_setopt(mcurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            AV_LOGI(LOG_TAG,
                    "RedCurl %p %s set ip downgrade "
                    "CURLOPT_IPRESOLVE: CURL_IPRESOLVE_V4",
                    this, __FUNCTION__);
          }
        }

        if (msg->data.result == CURLE_WRITE_ERROR) {
          AV_LOGW(LOG_TAG,
                  "RedCurl %p %s, write data error, exit CURLcode: %d\n", this,
                  __FUNCTION__, msg->data.result);
          curlres = CURLE_WRITE_ERROR;
          if (mdownpara->mopt->PreDownLoadSize > 0)
            reportNetworkQuality();
          goto failed;
        } else if (msg->data.result != CURLE_OK || httperr != 0 ||
                   (!mdownpara->preload_finished &&
                    (mdownpara->mopt->PreDownLoadSize > 0))) {
          m_curlcode = msg->data.result;
          if (msg->data.result == CURLE_PARTIAL_FILE) {
            mretrycount = 0;
          } else if (msg->data.result == CURLE_OPERATION_TIMEDOUT &&
                     mfilelength == 0) {
            AV_LOGI(LOG_TAG, "RedCurl %p %s, operation timeout\n", this,
                    __FUNCTION__);
            if (m_dns_status == DnsStatus::Waiting) {
              dnsCallback(nullptr, DnsStatus::Fail);
              curlres = ERROR_DNS_PARSE_FAIL;
            } else {
              curlres = ERROR_TCP_CONNECT_TIMEOUT;
            }
            reportNetworkQuality();
            curl_multi_remove_handle(multi_handle, mcurl);
            removeShareHandle();
            goto failed;
          } else if (msg->data.result == CURLE_COULDNT_RESOLVE_HOST) {
            AV_LOGI(LOG_TAG, "RedCurl %p %s, DNS fail\n", this, __FUNCTION__);
            dnsCallback(nullptr, DnsStatus::Fail);
            curlres = ERROR_DNS_PARSE_FAIL;
            reportNetworkQuality();
            curl_multi_remove_handle(multi_handle, mcurl);
            removeShareHandle();
            goto failed;
          } else if (httperr != 0) {
            AV_LOGI(LOG_TAG, "RedCurl %p %s, result http error %" PRId64 "\n", this,
                    __FUNCTION__, httpCode);
            reportNetworkQuality();
            curl_multi_remove_handle(multi_handle, mcurl);
            removeShareHandle();
            curlres = httperr;
            goto failed;
          }
          if (++mretrycount > max_retry) {
            reportNetworkQuality();
            curlres = ERROR(EIO);
            mretrycount = 0;
            goto failed;
          }
          if (mdownpara->downloadsize > mdownpara->range_end &&
              mdownpara->range_end != 0) {
            break;
          }
          reportNetworkQuality();
          std::string range("");
          if (mdownpara->range_end == 0) {
            range += (std::to_string(mdownpara->downloadsize) +
                      "-"); // todo(renwei) this may be a bug, if
                            // not start with 0
            AV_LOGI(LOG_TAG, "RedCurl %p %s, %p range %s, %" PRIu64 "--\n",
                    this, __FUNCTION__, mdownpara, range.c_str(),
                    mdownpara->downloadsize);
          } else {
            range += (std::to_string(mdownpara->downloadsize) + "-" +
                      std::to_string(mdownpara->range_end));
            AV_LOGI(LOG_TAG,
                    "RedCurl %p %s, %p range %s, %" PRIu64 "--%" PRIu64 "\n",
                    this, __FUNCTION__, mdownpara, range.c_str(),
                    mdownpara->downloadsize, mdownpara->range_end);
          }
          curl_multi_remove_handle(multi_handle, mcurl);
          curl_easy_setopt(mcurl, CURLOPT_RANGE, range.c_str());
          if (mretrycount == (max_retry / 2)) {
            removeShareHandle();
          }
          usleep(100000);
          curl_multi_add_handle(multi_handle, mcurl);
          // should add time to block
          return true;
        }
        if (mdownpara->mopt->PreDownLoadSize > 0)
          reportNetworkQuality();
      }
    }

    if (CURLResult == CURLE_OK || mdownpara->preload_finished) {
      if (downloadsize > 0 &&
          static_cast<int>(downloadsize) <
              static_cast<int>(mdownpara->range_end - mdownpara->range_start +
                               1)) {
        AV_LOGW(LOG_TAG,
                "RedCurl %p%s, reach eos, downloadsize %.2f, range "
                "start %" PRIu64 ", end %" PRIu64 "\n",
                this, __FUNCTION__, downloadsize, mdownpara->range_start,
                mdownpara->range_end);
        curlres = EOF;
      }
      mdownloadStatus->errorcode = 0;
      return false;
    }
  }

  return true;
failed:
  if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) > 0 &&
      !m_islive) {
    mdownloadStatus->errorcode = curlres;
    updateipaddr();
  }
  return false;
}

void RedCurl::removeShareHandle() {
  // AV_LOGI(LOG_TAG, "RedCurl %p %s\n", this, __FUNCTION__);
  if (RedDownloadConfig::getinstance()->get_config_value(SHAREDNS_KEY) > 0 ||
      m_islive) {
    if (m_islive && RedDownloadConfig::getinstance()->get_config_value(
                        SHAREDNS_LIVE_KEY) <= 0) {
      AV_LOGI(LOG_TAG, "RedCurl %p %s end\n", this, __FUNCTION__);
      return;
    }
    AV_LOGI(LOG_TAG, "%s, release sharehandle\n", __FUNCTION__);
    curl_easy_setopt(mcurl, CURLOPT_SHARE, NULL);
    std::lock_guard<std::mutex> lock(sharehandlemutex);
    if (share_handle && curl_share_cleanup(share_handle) == CURLSHE_OK) {
      share_handle = nullptr;
      AV_LOGI(LOG_TAG, "RedCurl %p %s, release sharehandle success\n", this,
              __FUNCTION__);
    }
  }
  AV_LOGI(LOG_TAG, "RedCurl %p %s end\n", this, __FUNCTION__);
}

int RedCurl::precreatetask(RedDownLoadPara *downpara) { return 0; }

int RedCurl::rundownload(RedDownLoadPara *downpara) {
  {
    std::unique_lock<std::mutex> locker(m_mutexCurl);
    AV_LOGI(LOG_TAG, "RedCurl %p %s,begin\n", this, __FUNCTION__);
    if (bpause.load())
      return false;
    brunning = true;
  }
  resetDownloadStatus();
  mdownpara = downpara;
  int err = 0;

  if (!prepare()) {
    brunning = false;
    return false;
  }

  mretrycount = 0;
  while (!bpause.load()) {
    m_curlcode = CURLE_OK;
    bool bcontinue = PerformCurl(err);
    if (!bcontinue)
      break;
  }

  if (err != 0 && err != CURLE_WRITE_ERROR) {
    if (mfilelength == 0) {
      HttpCallBack(err);
    }
    if (!bpause.load() && mdownpara != nullptr &&
        mdownpara->mdatacb != nullptr) {
      if (err > 0) {
        err = CURLERROR(err);
      }
      mdownpara->mdatacb->WriteData(
          nullptr, 0, reinterpret_cast<void *>(mdownpara), mserial, err);
      ErrorData data = err;
      eventCallback(RED_EVENT_ERROR, &data);
    }
  }
  {
    std::unique_lock<std::mutex> locker(m_mutexCurl);
    brunning = false;
    m_condition.notify_one();
  }
  if (bdescurl) {
    destroyCurl();
    AV_LOGW(LOG_TAG, "RedCurl %p %s, destroy libcurl\n", this, __FUNCTION__);
  }
  AV_LOGI(LOG_TAG, "RedCurl %p %s exit, ret %d\n", this, __FUNCTION__, err);
  return err;
}

size_t RedCurl::writefunc(uint8_t *buffer, size_t size, size_t nmemb,
                          void *userdata) {
  if (userdata == nullptr) {
    AV_LOGW(LOG_TAG, "%s, invalid userdata\n", __FUNCTION__);
    return size * nmemb - 1;
  }
  RedCurl *thiz = reinterpret_cast<RedCurl *>(userdata);
  if (thiz != nullptr && thiz->bpause.load()) {
    AV_LOGW(LOG_TAG, "RedCurl %p %s, abort\n", thiz, __FUNCTION__);
    return size * nmemb - 1;
  }
  if ((thiz != nullptr) && (thiz->mdownpara != nullptr) &&
      (thiz->mdownpara->mdatacb != nullptr) &&
      (thiz->mdownpara->mopt != nullptr)) {
    RedIOTraffic *mdata = new RedIOTraffic;
    mdata->obj = nullptr;
    mdata->bytes = static_cast<int>(size * nmemb);
    mdata->url = thiz->mdownpara->url;
    mdata->source = thiz->msource;
    thiz->mdownpara->mdatacb->DownloadCallBack(
        RED_EVENT_IO_TRAFFIC, reinterpret_cast<void *>(mdata), nullptr, 0, 0);
    delete mdata;
    mdata = nullptr;
    if (thiz->mdownloadStatus->httpcode != 0) {
      int64_t httpCode = 0;
      curl_easy_getinfo(thiz->mcurl, CURLINFO_RESPONSE_CODE, &httpCode);
      thiz->mdownloadStatus->httpcode = static_cast<int>(httpCode);
    }
    thiz->mdownloadStatus->downloadsize += (size * nmemb);
    size_t ret = thiz->mdownpara->mdatacb->WriteData(
        buffer, size * nmemb, reinterpret_cast<void *>(thiz->mdownpara),
        thiz->mserial, 0);
    if (thiz != nullptr && thiz->bpause.load()) {
      AV_LOGW(LOG_TAG, "RedCurl %p %s, abort\n", thiz, __FUNCTION__);
      return size * nmemb - 1;
    }
    thiz->mretrycount = 0;
    RedCacheStatistic *cachedata = new RedCacheStatistic;
    cachedata->cached_size = thiz->mdownpara->downloadsize;
    cachedata->logical_file_size = thiz->mfilelength;
    thiz->mdownpara->mdatacb->DownloadCallBack(
        RED_EVENT_CACHE_STATISTIC, reinterpret_cast<void *>(cachedata), nullptr,
        0, 0);
    delete cachedata;
    cachedata = nullptr;
    return ret;
  } else {
    AV_LOGW(LOG_TAG, "RedCurl %p%s, invalid callback interface\n", thiz,
            __FUNCTION__);
    return size * nmemb - 1;
  }
}

size_t RedCurl::headerfunc(void *buffer, size_t size, size_t nmemb,
                           void *userdata) {
  if (userdata == nullptr) {
    AV_LOGW(LOG_TAG, "%s, invalid userdata\n", __FUNCTION__);
    return size * nmemb - 1;
  }
  int r = 0;
  int64_t contentsize = 0;

  RedCurl *thiz = reinterpret_cast<RedCurl *>(userdata);
  if (thiz && !thiz->bpause.load() && !thiz->m_is_report_http_open &&
      thiz->mdownpara && thiz->mdownpara->mopt &&
      thiz->mdownpara->mopt->islive) {
    thiz->HttpCallBack(0);
    thiz->m_is_report_http_open = true;
  }

  if (!buffer)
    return size * nmemb;

  std::string bufferStr(static_cast<char *>(buffer));
  if (bufferStr.rfind("HTTP/", 0) != std::string::npos) {
    thiz->tcpCallback(0);
  }

  char range[1024] = {0};
  r = sscanf(reinterpret_cast<char *>(buffer), "Content-Range: bytes %100s\n",
             range);
  if (r > 0) {
    int len = static_cast<int>(strlen(range));
    int startpos = -1;
    for (int i = 0; i < len; ++i) {
      if (range[i] == '/') {
        startpos = i + 1;
        break;
      }
    }
    if (startpos > 0) {
      for (; startpos < len; ++startpos) {
        if (range[startpos] < '0' || range[startpos] > '9') {
          contentsize = 0;
          break;
        } else {
          contentsize = contentsize * 10 + range[startpos] - '0';
        }
      }
    }
    if (contentsize > 0) {
      if (thiz != nullptr && thiz->mdownpara != nullptr) {
        thiz->mdownpara->filesize = contentsize;
        if (thiz->mnotitystate == 1) {
          thiz->mnotitystate = 2;
        }
      }
    }
    AV_LOGW(LOG_TAG, "RedCurl %p %s, get content size %" PRId64 ", %p\n", thiz,
            __FUNCTION__, contentsize, userdata);
  }
  return size * nmemb;
}

int RedCurl::gethttperror(int http_code) {
  switch (http_code) {
  case 400:
    return ERROR_HTTP_BAD_REQUEST;
  case 401:
    return ERROR_HTTP_UNAUTHORIZED;
  case 403:
    return ERROR_HTTP_FORBIDDEN;
  case 404:
    return ERROR_HTTP_NOT_FOUND;
  default:
    break;
  }
  if (http_code >= 400 && http_code <= 499)
    return ERROR_HTTP_OTHER_4XX;
  else if (http_code >= 500)
    return ERROR_HTTP_SERVER_ERROR;
  else
    return 0;
}

int RedCurl::gettcperror(int tcp_code) {
  switch (tcp_code) {
  case CURLE_COULDNT_RESOLVE_PROXY:
  case CURLE_COULDNT_RESOLVE_HOST:
    return ERROR_DNS_PARSE_FAIL;
  case CURLE_COULDNT_CONNECT:
  case CURLE_OPERATION_TIMEDOUT:
    return ERROR_TCP_CONNECT_TIMEOUT;
  case CURLE_RECV_ERROR:
    return ERROR_TCP_READ_TIMEOUT;

  default:
    break;
  }

  return 0;
}

int RedCurl::getheadinfo() {
  int err = 0;
  CURLcode CURLResult = CURLE_OK;
  curl_easy_setopt(mcurl, CURLOPT_NOBODY, 1L);
  // curl_easy_setopt(mcurl, CURLOPT_CUSTOMREQUEST, "GET");
  // curl_easy_setopt(mcurl, CURLOPT_RANGE, "0-");
  curl_multi_add_handle(multi_handle, mcurl);
  if (mdownpara->mopt != nullptr && mdownpara->mdatacb != nullptr) {
    mdownpara->mdatacb->DownloadCallBack(RED_EVENT_WILL_HTTP_OPEN, nullptr,
                                         nullptr, 0, 0);
    if (mdownpara->mopt->tcp_buffer <= 0)
      mdownpara->mdatacb->DownloadCallBack(RED_CTRL_WILL_TCP_OPEN, nullptr,
                                           nullptr, 0, 0);
  }
  int64_t httpCode = 0;
  int retry_count = 0;
  while (!bpause.load()) {
    int still_running = 0;
    // CURLMcode code    = CURLM_OK;
    curl_multi_perform(multi_handle, &still_running);
    int numfds;
    curl_multi_wait(multi_handle, NULL, 0, 100, &numfds);
    if (!still_running) {
      int msgs;
      CURLMsg *msg;

      while ((msg = curl_multi_info_read(multi_handle, &msgs))) {
        if (msg->msg == CURLMSG_DONE) {
          if (msg->data.result == CURLE_OK ||
              msg->data.result == CURLE_WRITE_ERROR) {
            AV_LOGI(LOG_TAG, "RedCurl %p %s, get headerinfo ok\n", this,
                    __FUNCTION__);
            double length;
            if (curl_easy_getinfo(msg->easy_handle,
                                  CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                  &length) == CURLE_OK) {
              AV_LOGI(LOG_TAG, "RedCurl %p %s, get file size %.2f\n", this,
                      __FUNCTION__, length);
              mfilelength = length;
            }
          }
          CURLResult = msg->data.result;
          curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE,
                            &httpCode);
          if (msg->data.result == CURLE_HTTP_RETURNED_ERROR) {
            int err = gethttperror(static_cast<int>(httpCode));
            if (err != 0) {
              AV_LOGW(LOG_TAG, "RedCurl %p%s, returned error %" PRId64 "\n", this,
                      __FUNCTION__, httpCode);
              goto end;
            }
          }

          AV_LOGW(LOG_TAG, "RedCurl %p %s, curl_multi_info_read result %s(%d)",
                  this, __FUNCTION__, curl_easy_strerror(msg->data.result),
                  msg->data.result);

          if ((msg->data.result == CURLE_OPERATION_TIMEDOUT ||
               msg->data.result == CURLE_PARTIAL_FILE ||
               msg->data.result == CURLE_COULDNT_CONNECT ||
               msg->data.result == CURLE_RECV_ERROR ||
               msg->data.result == CURLE_COULDNT_RESOLVE_PROXY ||
               msg->data.result == CURLE_COULDNT_RESOLVE_HOST)) {
            ++retry_count;
            curl_multi_remove_handle(multi_handle, mcurl);
            curl_multi_add_handle(multi_handle, mcurl);
          } else if (msg->data.result != CURLE_OK) {
            AV_LOGW(LOG_TAG, "RedCurl %p unkonwn error %d, reutrn\n", this,
                    msg->data.result);
            retry_count = max_retry + 1;
          }
        }
      }

      if (CURLResult == CURLE_OK || CURLResult == CURLE_WRITE_ERROR) {
        AV_LOGW(LOG_TAG, "RedCurl %pget head info finished\n", this);
        break;
      } else if (retry_count > max_retry) {
        AV_LOGW(LOG_TAG, "RedCurl %p retry count  %d, return \n", this,
                retry_count);
        int error_code = static_cast<int>(CURLResult);
        err = gettcperror(error_code);
        if (err == 0) {
          err = error_code;
        }
        if (err > 0) {
          err = CURLERROR(err);
        }
        break;
      }
    }
  }
end:
  if (err != 0) {
    if (mdownpara != nullptr && mdownpara->mdatacb != nullptr) {
      if (err > 0) {
        err = CURLERROR(err);
      }
      size_t ret =
          mdownpara->mdatacb->WriteData(nullptr, 0, nullptr, mserial, err);
      return static_cast<int>(ret);
    }
  }
  return err;
}

uint64_t RedCurl::getlength() { return mfilelength; }

bool RedCurl::pause(bool block) {
  std::unique_lock<std::mutex> locker(m_mutexCurl);
  if (bpause.load())
    return true;
  bpause.store(true);
  if (mdownpara && mdownpara->mdatacb && mdownpara->mopt &&
      !mdownpara->mopt->islive) {
    UrlParser up(mdownpara->url);
    std::string cache_path = mdownpara->mopt->cache_file_dir + up.geturi();
    RedIOTraffic *mdata = new RedIOTraffic;
    mdata->obj = nullptr;
    mdata->bytes = static_cast<int>(mdownloadStatus->downloadsize.load());
    mdata->url = mdownpara->url;
    mdata->source = msource;
    mdata->cache_size = mdata->bytes;
    mdata->cache_path = cache_path;
    mdownpara->mdatacb->DownloadCallBack(RED_EVENT_DID_FRAGMENT_COMPLETE,
                                         reinterpret_cast<void *>(mdata),
                                         nullptr, 0, 0);
    delete mdata;
    mdata = nullptr;
  }
  if (mdownpara && mdownpara->mdatacb && mdownpara->mopt &&
      mdownpara->mopt->PreDownLoadSize > 0) {
    if (m_endtime.load() == 0)
      m_endtime = CurrentTimeUs();
    int64_t duration = m_endtime.load() - m_starttime.load();
    if (mdownloadStatus->downloadsize.load() > 0 && duration > 0) {
      int64_t downloadspeed = (int64_t)mdownloadStatus->downloadsize.load() *
                              1000000 * 8 / duration; // bits/second
      AV_LOGI(LOG_TAG, "RedCurl %p download speed: %" PRId64 "b/s\n", this,
              downloadspeed);
      mdownpara->mdatacb->DownloadCallBack(
          RED_SPEED_CALL_BACK, nullptr, nullptr,
          mdownloadStatus->downloadsize.load(), downloadspeed);
    }
  }
  AV_LOGI(LOG_TAG, "RedCurl %p download size: %" PRId64 " bytes\n", this,
          mdownloadStatus->downloadsize.load());

  if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) > 0 &&
      !m_islive) {
    updateipaddr();
  }

  if (block) {
    if (brunning) {
      // int waittimei = mdownpara->mopt->connect_timeout + 500;
      if (m_condition.wait_for(locker, std::chrono::milliseconds(1000)) ==
          std::cv_status::timeout) {
        m_pause_timeout.store(true);
        AV_LOGW(LOG_TAG, "RedCurl %p pause timeout\n", this);
      }
    }
  }
  // AV_LOGW(LOG_TAG, "RedCurl %p pause return %d\n", this, brunning);
  return true;
}

void RedCurl::stop() { AV_LOGW(LOG_TAG, "RedCurl %p stop\n", this); }

void RedCurl::updateipaddr() {
  int err = mdownloadStatus->errorcode.load();
  if (err == 0 || err == CURLE_WRITE_ERROR) {
    if (m_endtime.load() == 0)
      m_endtime = CurrentTimeUs();
    int64_t duration = m_endtime.load() - m_starttime.load();
    int64_t downloadspeed = 0;
    if (mdownloadStatus->downloadsize.load() > 0 && duration > 0 && mdownpara &&
        mdownpara->mdatacb && mdownpara->mopt &&
        mdownpara->mopt->PreDownLoadSize > 0) {
      downloadspeed =
          (int64_t)mdownloadStatus->downloadsize.load() * 1000000 / duration;
    }
    REDDnsCache::getinstance()->UpdateAddr(mhostname, mcuripaddr, downloadspeed,
                                           0);
  } else {
    REDDnsCache::getinstance()->UpdateAddr(mhostname, mcuripaddr, 0, 1);
  }
}

bool RedCurl::resume() {
  AV_LOGW(LOG_TAG, "RedCurl %p resume !!!\n", this);
  if (bpause.load()) {
    bpause.store(false);
    m_pause_timeout.store(false);
  }
  return true;
}

int RedCurl::updatepara(RedDownLoadPara *downpara, bool neednotify) {
  mdownpara = downpara;
  updateCurl();
  resetDownloadStatus();
  if (neednotify) {
    mnotitystate = 1;
    if (mdownpara->mopt != nullptr && mdownpara->mdatacb != nullptr) {
      mdownpara->mdatacb->DownloadCallBack(RED_EVENT_WILL_HTTP_OPEN, nullptr,
                                           nullptr, 0, 0);
      mhttpStartTs = getTimestampMs();
      mdownpara->mdatacb->DownloadCallBack(RED_CTRL_WILL_TCP_OPEN, nullptr,
                                           nullptr, 0, 0);
      mtcpStartTs = mhttpStartTs;
    }
  }
  return 0;
}

void RedCurl::continuedownload() {
  std::unique_lock<std::mutex> lock(m_mutex_infinite_range);
  m_continue_infinite_range = true;
  m_con_infinite_range.notify_all();
}

void RedCurl::dnsCallback(struct curl_sockaddr *address, DnsStatus status) {
  if (bpause.load() || !mdownpara || !mdownpara->mdatacb) {
    AV_LOGI(LOG_TAG, "RedCurl %p %s, pause, return\n", this, __FUNCTION__);
    return;
  }
  switch (status) {
  case DnsStatus::Waiting: {
    AV_LOGI(LOG_TAG, "RedCurl %p %s, dns wait, pre read dns %d, url:%s\n", this,
            __FUNCTION__, m_is_read_dns.load(), mdownpara->url.c_str());
    m_dns_status = status;
    auto dnsInfo = std::make_shared<RedDnsInfo>();
    UrlParser up(mdownpara->url);
    dnsInfo->domain = up.getdomain();
    mdownpara->mdatacb->DownloadCallBack(RED_EVENT_WILL_DNS_PARSE,
                                         dnsInfo.get(), nullptr, 0, 0);
  } break;
  case DnsStatus::Success: {
    AV_LOGI(LOG_TAG, "RedCurl %p %s, dns success, pre read dns %d, url:%s\n",
            this, __FUNCTION__, m_is_read_dns.load(), mdownpara->url.c_str());
    if (!address)
      break;
    m_dns_status = status;
    auto dnsInfo = std::make_shared<RedDnsInfo>();
    UrlParser up(mdownpara->url);
    dnsInfo->domain = up.getdomain();
    dnsInfo->status = 1;
    struct sockaddr addr = address->addr;
    dnsInfo->family = addr.sa_family;
    struct sockaddr_storage so_stg;
    if (address->addrlen <= sizeof(so_stg)) {
      memset(&so_stg, 0, sizeof(so_stg));
      memcpy(&so_stg, &(address->addr), address->addrlen);

      switch (dnsInfo->family) {
      case AF_INET: {
        struct sockaddr_in *in4 = (struct sockaddr_in *)&so_stg;
        char ip[96]{0};
        if (inet_ntop(AF_INET, &(in4->sin_addr), ip, sizeof(ip))) {
          dnsInfo->ip = ip;
          dnsInfo->port = in4->sin_port;
        }
        break;
      }
      case AF_INET6: {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&so_stg;
        char ip[96]{0};
        if (inet_ntop(AF_INET6, &(in6->sin6_addr), ip, sizeof(ip))) {
          dnsInfo->ip = ip;
          dnsInfo->port = in6->sin6_port;
        }
        break;
      }
      }
      mdownpara->mdatacb->DownloadCallBack(RED_EVENT_DID_DNS_PARSE,
                                           dnsInfo.get(), nullptr, 0, 0);
    }
  } break;
  case DnsStatus::Fail: {
    if (m_dns_status != DnsStatus::Waiting)
      break;
    m_dns_status = status;
    auto dnsInfo = std::make_shared<RedDnsInfo>();
    UrlParser up(mdownpara->url);
    dnsInfo->domain = up.getdomain();
    dnsInfo->status = 0;
    mdownpara->mdatacb->DownloadCallBack(RED_EVENT_DID_DNS_PARSE, dnsInfo.get(),
                                         nullptr, 0, 0);
  } break;
  default:
    break;
  }
}

void RedCurl::reportNetworkQuality() {
  std::shared_ptr<NQIndicator> indicator = std::make_shared<NQIndicator>();
  bool report = false; // if to report
  do {
    // calculate tcp rtt
    double dnsTs = 0;
    curl_easy_getinfo(mcurl, CURLINFO_NAMELOOKUP_TIME, &dnsTs);
    if (dnsTs == 0) {
      if (m_dns_status == DnsStatus::Fail) {
        indicator->exception = NQException::DNS;
        report = true;
      }
      break;
    }
    double tcpTs = 0;
    curl_easy_getinfo(mcurl, CURLINFO_CONNECT_TIME, &tcpTs);
    if (tcpTs == 0) {
      if (m_curlcode != CURLE_OK) {
        indicator->exception = NQException::TCP;
        report = true;
      }
      break;
    }
    indicator->tcpRTT = (tcpTs - dnsTs) * 1000;

    // calculate http rtt
    double sslTs = 0;
    double pretransferTs = 0;
    double httpTs = 0;
    curl_easy_getinfo(mcurl, CURLINFO_APPCONNECT_TIME, &sslTs);
    curl_easy_getinfo(mcurl, CURLINFO_PRETRANSFER_TIME, &pretransferTs);
    curl_easy_getinfo(mcurl, CURLINFO_STARTTRANSFER_TIME, &httpTs);
    if (httpTs == 0 || pretransferTs == 0) {
      if (m_curlcode != CURLE_OK) {
        indicator->exception = NQException::HTTP;
        report = true;
      }
      break;
    }
    if (sslTs > 0) // https
      indicator->httpRTT = ((sslTs - tcpTs) + (httpTs - pretransferTs)) * 1000;
    else
      indicator->httpRTT = (httpTs - pretransferTs) * 1000;

    if (m_curlcode != CURLE_OK) {
      indicator->exception = NQException::TRANSFER;
      report = true;
      break;
    }

    // calculate download speed(credible when size > 32KB)
    if (mdownloadStatus->downloadsize.load() > 32 * 1024) {
      double downloadSpeed = 0;
      curl_easy_getinfo(mcurl, CURLINFO_SPEED_DOWNLOAD, &downloadSpeed);
      if (downloadSpeed == 0)
        break;
      indicator->downloadSpeed = downloadSpeed;
    } else {
      break;
    }
    report = true;
  } while (false);
  if (report)
    NetworkQuality::getInstance()->addIndicator(indicator);
}
