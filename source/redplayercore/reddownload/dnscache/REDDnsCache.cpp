#include "REDDnsCache.h"

#include <inttypes.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <json.hpp>

#include "REDURLParser.h"
#include "RedDownloadConfig.h"
#include "RedLog.h"
#include "curl/curl.h"
#include "utility/Utility.h"

#define LOG_TAG "RedDnsCache"

using Json = nlohmann::json;

static std::once_flag RedDnsCacheOnceFlag;
REDDnsCache *REDDnsCache::minstance = nullptr;

REDDnsCache *REDDnsCache::getinstance() {
  std::call_once(RedDnsCacheOnceFlag,
                 [&] { minstance = new (std::nothrow) REDDnsCache(); });
  return minstance;
}

REDDnsCache::REDDnsCache() : m_mutex(), m_cond() {
  mtimescale = 0; // ms
  babort = false;
  mdownloadcb = NULL;

  {
    HostData *hostdata = new HostData;
    hostdata->userdata =
        new UserData("sns-video-qc.xhscdn.com", reinterpret_cast<void *>(this));
    mdnscache["sns-video-qc.xhscdn.com"] = hostdata;
  }
  {
    HostData *hostdata = new HostData;
    hostdata->userdata =
        new UserData("sns-video-hw.xhscdn.com", reinterpret_cast<void *>(this));
    mdnscache["sns-video-hw.xhscdn.com"] = hostdata;
  }
  {
    HostData *hostdata = new HostData;
    hostdata->userdata =
        new UserData("sns-video-bd.xhscdn.com", reinterpret_cast<void *>(this));
    mdnscache["sns-video-bd.xhscdn.com"] = hostdata;
  }
  {
    HostData *hostdata = new HostData;
    hostdata->userdata =
        new UserData("sns-video-al.xhscdn.com", reinterpret_cast<void *>(this));
    mdnscache["sns-video-al.xhscdn.com"] = hostdata;
  }
  ares_library_init(ARES_LIB_INIT_ALL);
  mhttpdns = RedDownloadConfig::getinstance()->get_config_value(HTTPDNS_KEY);
}

const struct ares_socket_functions REDDnsCache::default_functions = {
    [](int af, int type, int protocol, void *) -> ares_socket_t {
      auto s = socket(af, type, protocol);
      AV_LOGW(LOG_TAG, "ares_socket_functions socket %d\n", s);
      if (s == ARES_SOCKET_BAD) {
        return s;
      } else if (s >= 1023) {
        close(s);
        return ARES_SOCKET_BAD;
      }
      struct timeval timeo;
      timeo.tv_sec = MAX_DNSCACHE_PING_TIMEOUT;
      timeo.tv_usec = 0;
      if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1) {
        AV_LOGW(LOG_TAG, "ares_socket_functions set socket send err, fd %d\n",
                s);
        close(s);
        return ARES_SOCKET_BAD;
      }
      if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo)) == -1) {
        AV_LOGW(LOG_TAG, "ares_socket_functions set socket rec err, fd %d\n",
                s);
        close(s);
        return ARES_SOCKET_BAD;
      }
      return s;
    },
    [](ares_socket_t s, void *p) { return close(s); },
    [](ares_socket_t s, const struct sockaddr *addr, socklen_t len, void *) {
      return connect(s, addr, len);
    },
    [](ares_socket_t s, void *dst, size_t len, int flags, struct sockaddr *addr,
       socklen_t *alen, void *) -> ares_ssize_t {
      return recvfrom(s, reinterpret_cast<void *>(dst), len, flags, addr, alen);
    },
    [](ares_socket_t s, const struct iovec *vec, int len, void *) {
      return writev(s, vec, len);
    }};

void REDDnsCache::SetDnsCb(DownLoadListen *cb) {
  std::unique_lock<std::mutex> lock(m_mutex);
  mdownloadcb = cb;
  AV_LOGW(LOG_TAG, "%s, %p\n", __FUNCTION__, cb);
}

void REDDnsCache::SetHostDnsServerIp(string dnsserver) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (dnsserver.empty())
    return;
  mdnsderverip = dnsserver;
  AV_LOGW(LOG_TAG, "%s, %s\n", __FUNCTION__, dnsserver.c_str());
}

string REDDnsCache::GetHostDnsServerIp() {
  string res;
  if (mdownloadcb) {
    AV_LOGW(LOG_TAG, "%s, ares dnscb %p\n", __FUNCTION__, mdownloadcb);
    char serverip[4096] = {0};
    int ret = mdownloadcb->DnsCb(serverip, sizeof(serverip));
    if (ret > 0) {
      res = serverip;
    }
    if (res != mdnsderverip) {
      AV_LOGW(LOG_TAG, "%s, dnsserverip changed from %s to %s\n", __FUNCTION__,
              mdnsderverip.c_str(), res.c_str());
      mdnsderverip = res;
      ClearDnsCache();
    }
  }
  return res;
}

void REDDnsCache::NetWorkChanged() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cond.notify_all();
}

REDDnsCache::~REDDnsCache() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    babort = true;
    m_cond.notify_all();
  }
  if (mthread != nullptr && mthread->joinable()) {
    mthread->join();
    delete mthread;
    mthread = nullptr;
  }
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto &mapinter : mdnscache) {
      mapinter.second->vecipinfo.clear();
      HostData *hostdata = mapinter.second;
      if (hostdata != nullptr) {
        if (hostdata->userdata != nullptr) {
          delete hostdata->userdata;
        }
        delete hostdata;
      }
      mapinter.second = nullptr;
    }
  }

  ares_library_cleanup();
}

void REDDnsCache::ClearDnsCache() {
  std::unique_lock<std::mutex> lock(m_mutex);
  for (auto &mapinter : mdnscache) {
    mapinter.second->vecipinfo.clear();
  }
}

void REDDnsCache::ClearFdInfo() {
  for (auto &iter : mfdinfo) {
    if (iter.fd > 0) {
      close(iter.fd);
    }
    iter.fd = 0;
  }
  mfdinfo.clear();
}
void REDDnsCache::AddHost(string hostname) {
  std::unique_lock<std::mutex> lock(m_mutex);
  std::unordered_map<string, HostData *>::iterator infoMapIter;
  if (hostname.empty() ||
      (infoMapIter = mdnscache.find(hostname)) != mdnscache.end()) {
    return;
  }
  HostData *hostdata = new HostData;
  hostdata->userdata = new UserData(hostname, reinterpret_cast<void *>(this));
  mdnscache[hostname] = hostdata;
}

void REDDnsCache::SetTimescale(int timescale) {
  std::unique_lock<std::mutex> lock(m_mutex);
  mtimescale = timescale;
  AV_LOGW(LOG_TAG, "SetTimescale %d\n", timescale);
}
string REDDnsCache::GetValidHost() {
  std::unique_lock<std::mutex> lock(m_mutex);
  return mvalidhost;
}
void REDDnsCache::UpdateAddr(string hostname, string curip, int64_t speed,
                             int err) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (hostname.empty() || curip.empty())
    return;
  if (err == 0)
    mvalidhost = hostname;
  AV_LOGW(LOG_TAG,
          "UpdateIpAddr hostname %s, ip %s, speed %" PRId64 ", err %d!\n",
          hostname.c_str(), curip.c_str(), speed, err);
  std::unordered_map<string, HostData *>::iterator infoMapIter;
  if ((infoMapIter = mdnscache.find(hostname)) != mdnscache.end()) {
    HostData *hostdata = infoMapIter->second;
    vector<IpAddrInfo>::iterator iter = hostdata->vecipinfo.begin();
    for (; iter != hostdata->vecipinfo.end(); ++iter) {
      if (strcmp(iter->ipaddr.c_str(), curip.c_str()) == 0) {
        if (speed > 0)
          iter->speed = speed;
        iter->usecounts++;
        if (err == 0) {
          iter->successcounts++;
          iter->bvalid = true;
        }
        break;
      }
    }
    if (iter == hostdata->vecipinfo.end() && err == 0) {
      IpAddrInfo ipaddrinfo;
      ipaddrinfo.ipaddr = curip;
      ipaddrinfo.bvalid = true;
      if (strchr(curip.c_str(), ':') != NULL) {
        ipaddrinfo.family = AF_INET6;
      } else {
        ipaddrinfo.family = AF_INET;
      }
      if (speed > 0)
        ipaddrinfo.speed = speed;
      ipaddrinfo.usecounts++;
      ipaddrinfo.successcounts++;
      hostdata->vecipinfo.push_back(ipaddrinfo);
    }
  } else if (err == 0) {
    IpAddrInfo ipaddrinfo;
    ipaddrinfo.ipaddr = curip;
    ipaddrinfo.bvalid = true;
    if (strchr(curip.c_str(), ':') != NULL) {
      ipaddrinfo.family = AF_INET6;
    } else {
      ipaddrinfo.family = AF_INET;
    }
    if (speed > 0)
      ipaddrinfo.speed = speed;
    ipaddrinfo.usecounts++;
    ipaddrinfo.successcounts++;
    HostData *hostdata = new HostData;
    hostdata->userdata = new UserData(hostname, reinterpret_cast<void *>(this));
    hostdata->vecipinfo.push_back(ipaddrinfo);
    mdnscache[hostname] = hostdata;
  }
  if (err) {
    m_cond.notify_all();
  }
}

string REDDnsCache::GetIpAddr(string hostname) {
  std::unique_lock<std::mutex> lock(m_mutex);
  string ipaddr;
  double maxscore = 0;
  int64_t maxspeed = 0;
  int minrtt = INT_MAX;
  if (RedDownloadConfig::getinstance()->get_config_value(FORCE_USE_IPV6) > 0) {
    string ipaddr4;
    double maxscorev4 = 0;
    int64_t maxspeedv4 = 0;
    int minrttv4 = INT_MAX;
    string ipaddr6;
    double maxscorev6 = 0;
    int64_t maxspeedv6 = 0;
    int minrttv6 = INT_MAX;
    std::unordered_map<string, HostData *>::iterator infoMapIter;
    if ((infoMapIter = mdnscache.find(hostname)) != mdnscache.end()) {
      // find best ipaddr
      HostData *hostdata = infoMapIter->second;
      vector<IpAddrInfo>::iterator iter = hostdata->vecipinfo.begin();
      for (; iter != hostdata->vecipinfo.end(); ++iter) {
        if (!iter->bvalid)
          continue;
        if (iter->family == AF_INET6) {
          double score =
              static_cast<double>(iter->successcounts) / (iter->usecounts);
          if (score > maxscorev6 ||
              (score == maxscorev6 && iter->speed > maxspeedv6) ||
              ((score == maxscorev6) && (iter->speed == maxspeedv6) &&
               (iter->rtt < minrttv6))) {
            maxscorev6 = score;
            maxspeedv6 = iter->speed;
            minrttv6 = iter->rtt;
            ipaddr6 = iter->ipaddr;
          }
        } else {
          double score =
              static_cast<double>(iter->successcounts) / (iter->usecounts);
          if (score > maxscorev4 ||
              (score == maxscorev4 && iter->speed > maxspeedv4) ||
              ((score == maxscorev4) && (iter->speed == maxspeedv4) &&
               (iter->rtt < minrttv4))) {
            maxscorev4 = score;
            maxspeedv4 = iter->speed;
            minrttv4 = iter->rtt;
            ipaddr4 = iter->ipaddr;
          }
        }
      }
    }
    if (maxscorev6 < maxscorev4) {
      ipaddr = ipaddr4;
      maxscore = maxscorev4;
      maxspeed = maxspeedv4;
      minrtt = minrttv4;
    } else {
      ipaddr = ipaddr6;
      maxscore = maxscorev6;
      maxspeed = maxspeedv6;
      minrtt = minrttv6;
    }
  } else {
    std::unordered_map<string, HostData *>::iterator infoMapIter;
    if ((infoMapIter = mdnscache.find(hostname)) != mdnscache.end()) {
      // find best ipaddr
      HostData *hostdata = infoMapIter->second;
      vector<IpAddrInfo>::iterator iter = hostdata->vecipinfo.begin();
      for (; iter != hostdata->vecipinfo.end(); ++iter) {
        if (!iter->bvalid)
          continue;
        double score =
            static_cast<double>(iter->successcounts) / (iter->usecounts);
        if (score > maxscore || (score == maxscore && iter->speed > maxspeed) ||
            ((score == maxscore) && (iter->speed == maxspeed) &&
             (iter->rtt < minrtt))) {
          maxscore = score;
          maxspeed = iter->speed;
          minrtt = iter->rtt;
          ipaddr = iter->ipaddr;
        }
      }
    }
  }
  AV_LOGW(LOG_TAG,
          "%s, hostname %s ip %s score %.2f, speed %" PRId64 ", rtt %d\n",
          __FUNCTION__, hostname.c_str(), ipaddr.c_str(), maxscore, maxspeed,
          minrtt);
  return ipaddr;
}

void REDDnsCache::run() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (babort) {
      AV_LOGW(LOG_TAG, "%p REDDnsCacheThread abort!\n", this);
      return;
    }
  }
  mthread = new std::thread(
      std::bind(&REDDnsCache::RunLoop, this, "REDDnsCacheThread"));
}

void REDDnsCache::RunLoop(std::string thread_name) {
#ifdef __APPLE__
  pthread_setname_np(thread_name.c_str());
#elif __ANDROID__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#elif __HARMONY__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
  AV_LOGW(LOG_TAG, "run thread %p begin!\n", this);
  do {
    ParseDns();
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (babort)
        return;
      if (mtimescale > 0)
        m_cond.wait_for(lock, std::chrono::milliseconds(mtimescale));
      else
        m_cond.wait(lock);
      if (babort)
        return;
    }
  } while (1);
}

void REDDnsCache::ParseDns() {
  vector<string> hostnames;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto &mapinter : mdnscache) {
      hostnames.push_back(mapinter.first);
    }
  }
  switch (mhttpdns) {
  case 0:
    ParseDnsCares(hostnames);
    break;
  case 1:
    GetHttpDns(hostnames);
    WaitHttpDnsPing();
    break;
  case 2:
    GetHttpDns(hostnames);
    ParseDnsCares(hostnames);
  default:
    break;
  }
}

void REDDnsCache::ParseDnsLocal(string hostname) {
  if (hostname.empty()) {
    return;
  }
  AV_LOGW(LOG_TAG, "%s, hostname %s \n", __FUNCTION__, hostname.c_str());
  // UrlParser up(prohostname);
  // string hostname = up.getdomain();
  // string prototol = up.getprotocol();
  const char *portstr = "80";
  // if (strcmp(prototol.c_str(), "https") == 0) {
  //     portstr = "443";
  // }
  // get ipaddr
  struct addrinfo hints = {0}, *ai = nullptr, *priai = nullptr;
  AV_LOGW(LOG_TAG, "%s, hostname %s 1\n", __FUNCTION__, hostname.c_str());
  int ret = getaddrinfo(hostname.c_str(), portstr, &hints, &ai);
  AV_LOGW(LOG_TAG, "%s, hostname %s 2\n", __FUNCTION__, hostname.c_str());
  if (ret) {
    AV_LOGW(LOG_TAG, "%s, resolve hostname %s failed!\n", __FUNCTION__,
            hostname.c_str());
    return;
  }
  if (ai)
    priai = ai;
  while (ai) {
    struct sockaddr_storage so_stg;
    char ip[96] = {0};
    if (ai->ai_addr && ai->ai_addrlen <= sizeof(so_stg)) {
      memset(&so_stg, 0, sizeof(so_stg));
      memcpy(&so_stg, ai->ai_addr, ai->ai_addrlen);

      int so_family;
      int family = AF_INET;
      so_family = ((struct sockaddr *)&so_stg)->sa_family;
      switch (so_family) {
      case AF_INET: {
        struct sockaddr_in *in4 = (struct sockaddr_in *)&so_stg;
        if (inet_ntop(AF_INET, &(in4->sin_addr), ip, sizeof(ip))) {
          family = AF_INET;
        }
        break;
      }
      case AF_INET6: {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&so_stg;
        if (inet_ntop(AF_INET6, &(in6->sin6_addr), ip, sizeof(ip))) {
          family = AF_INET6;
        }
        break;
      }
      default:
        break;
      }

      if (strlen(ip) > 0) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (mdnscache.find(hostname) != mdnscache.end()) {
          HostData *hostdata = mdnscache[hostname];
          vector<IpAddrInfo>::iterator iter = hostdata->vecipinfo.begin();
          for (; iter != hostdata->vecipinfo.end(); ++iter) {
            if (strcmp(iter->ipaddr.c_str(), ip) == 0) {
              break;
            }
          }
          if (iter == hostdata->vecipinfo.end()) {
            IpAddrInfo ipaddrinfo;
            ipaddrinfo.ipaddr = ip;
            ipaddrinfo.family = family;
            hostdata->vecipinfo.push_back(ipaddrinfo);
          }
        }
      }
      AV_LOGW(LOG_TAG, "%s, ip %s \n", __FUNCTION__, ip);
      ai = ai->ai_next;
    }
  }
  if (priai)
    freeaddrinfo(priai);
  return;
}

void REDDnsCache::ParseDnsCares(vector<string> &hostnames) {
  AV_LOGW(LOG_TAG, "%s cares parse  begin!\n", __FUNCTION__);
  ares_channel channel;
  int res;
  if ((res = ares_init(&channel)) != ARES_SUCCESS) {
    AV_LOGW(LOG_TAG, "%s, ares init failed!\n", __FUNCTION__);
    return;
  }
#ifdef __ANDROID__
  string dnsservers = GetHostDnsServerIp();
  if (!dnsservers.empty()) {
    int ret = ares_set_servers_csv(channel, dnsservers.c_str());
    if (ret != ARES_SUCCESS) {
      ares_destroy(channel);
      return;
    }
  } else {
    AV_LOGE(LOG_TAG, "%s, ares get dnsserver failed!\n", __FUNCTION__);
  }
#endif
  ares_set_socket_functions(channel, &REDDnsCache::default_functions, nullptr);
  for (auto hostname : hostnames) {
    UserData *userdata = mdnscache[hostname]->userdata;
    // userdata->hostname = hostname.c_str();
    userdata->cacheptr = reinterpret_cast<void *>(this);
    ares_gethostbyname(channel, hostname.c_str(), AF_INET,
                       &REDDnsCache::Cares_Cb,
                       reinterpret_cast<void *>(userdata));
    ares_gethostbyname(channel, hostname.c_str(), AF_INET6,
                       &REDDnsCache::Cares_Cb,
                       reinterpret_cast<void *>(userdata));
  }
  int count = 1;
  int nfds = 0;
  fd_set readers, writers;
  timeval tv, *tvp, maxtv;
  do {
    count = 1;
    FD_ZERO(&readers);
    FD_ZERO(&writers);
    nfds = ares_fds(channel, &readers, &writers);
    int cachefds = Add_fds(&readers, &writers);
    if (nfds <= 0 && cachefds <= 0) {
      break;
    }
    int maxfd = max(nfds, cachefds + 1);
    if (nfds > 0) {
      maxtv.tv_sec = MAX_ARES_PARSE_TIMEOUT;
      maxtv.tv_usec = 0;
      tvp = ares_timeout(channel, &maxtv, &tv);
      count = select(maxfd, &readers, &writers, NULL, tvp);
    } else {
      maxtv.tv_sec = MAX_DNSCACHE_PING_TIMEOUT;
      maxtv.tv_usec = 0;
      count = select(maxfd, &readers, &writers, NULL, &maxtv);
    }
    if (count < 0) {
      break;
    } else if (count == 0) {
      AV_LOGW(LOG_TAG, "%s cares parse count %d !\n", __FUNCTION__, count);
      ResetPing();
    }
    ares_process(channel, &readers, &writers);
    // check if ping
    Get_fds(&readers, &writers);
  } while (1);
  ares_destroy(channel);
  ClearFdInfo();
  AV_LOGW(LOG_TAG, "%s cares parse count %d end!\n", __FUNCTION__, count);
  return;
}

void REDDnsCache::Get_fds(fd_set *readers, fd_set *writers) {
  for (auto &iter : mfdinfo) {
    if (iter.fd > 0) {
      if (FD_ISSET(iter.fd, readers)) {
        RecvPingInfo(iter.fd, &iter);
        break;
      }
    }
  }
}

int REDDnsCache::Add_fds(fd_set *readers, fd_set *writers) {
  int cachefds = 0;
  for (auto &iter : mfdinfo) {
    if (iter.fd > 0) {
      cachefds = cachefds > iter.fd ? cachefds : iter.fd;
      FD_SET(iter.fd, readers);
    }
  }
  return cachefds;
}

void REDDnsCache::WaitHttpDnsPing() {
  fd_set readers, writers;
  timeval maxtv;
  do {
    FD_ZERO(&readers);
    FD_ZERO(&writers);
    int cachefds = Add_fds(&readers, &writers);
    if (cachefds <= 0) {
      break;
    }
    int maxfd = cachefds + 1;
    maxtv.tv_sec = MAX_DNSCACHE_PING_TIMEOUT;
    maxtv.tv_usec = 0;
    int count = select(maxfd, &readers, &writers, NULL, &maxtv);

    if (count < 0) {
      break;
    } else if (count == 0) {
      AV_LOGW(LOG_TAG, "%s httpdns parse count %d !\n", __FUNCTION__, count);
      ResetPing();
      continue;
    }
    Get_fds(&readers, &writers);
  } while (1);
  ClearFdInfo();
  AV_LOGW(LOG_TAG, "%s httpdns parse end!\n", __FUNCTION__);
}

void REDDnsCache::ResetPing() {
  for (auto &iter : mfdinfo) {
    if (iter.count < MAX_DNSCACHE_PING_COUNT) {
      PushAddrInfo(&iter, MAX_DNSCACHE_RTT_DURATION);
      ++iter.count;
      iter.fd = SetPing(iter.ipaddr.c_str(), iter.family, iter.fd, iter.count);
      struct timeval time_cur;
      gettimeofday(&time_cur, NULL);
      iter.lasttime = time_cur.tv_sec;
    } else if (iter.fd > 0) {
      close(iter.fd);
      iter.fd = 0;
    }
  }
}

void REDDnsCache::Cares_Cb(void *userdata, int status, int timeout,
                           struct hostent *addrinfo) {
  if (userdata == NULL)
    return;
  UserData *data = reinterpret_cast<UserData *>(userdata);
  REDDnsCache *thiz = reinterpret_cast<REDDnsCache *>(data->cacheptr);
  if (status == ARES_SUCCESS) {
    thiz->PreParePing(data->hostname, addrinfo);
  } else {
    AV_LOGW(LOG_TAG, "%s cares parse hostname %s failed, ret %d!\n",
            __FUNCTION__, data->hostname.c_str(), status);
  }
}

void REDDnsCache::GetHttpDns(vector<string> &hostnames) { // TODO: add ttl
  std::call_once(globalCurlInitOnceFlag,
                 [&] { curl_global_init(CURL_GLOBAL_ALL); });
  std::string httpdnsRequestUrl{httpdnsServer};
  auto hostNum = hostnames.size();
  for (int index = 0; index < hostNum; ++index) {
    httpdnsRequestUrl += hostnames[index];
    if (index != hostNum - 1)
      httpdnsRequestUrl += ",";
  }
  CURL *curl = nullptr;
  do {
    curl = curl_easy_init();
    if (!curl)
      break;
    int retCode =
        curl_easy_setopt(curl, CURLOPT_URL, httpdnsRequestUrl.c_str());
    if (strncmp(httpdnsServer, "https://", 8) == 0) {
      retCode += curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      retCode += curl_easy_setopt(curl, CURLOPT_PROXY_SSL_VERIFYHOST, 0L);
      retCode += curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      retCode += curl_easy_setopt(curl, CURLOPT_PROXY_SSL_VERIFYPEER, 0L);
      retCode += curl_easy_setopt(curl, CURLOPT_PROXY_SSL_VERIFYPEER, 0L);
    }
    retCode += curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    retCode += curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    retCode += curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    auto writeFunc = +[](uint8_t *buffer, size_t size, size_t nmemb,
                         void *userdata) -> size_t {
      if (!userdata)
        return size * nmemb - 1;
      std::string *buf = (std::string *)userdata;
      *buf += std::string(reinterpret_cast<char *>(buffer), size * nmemb);
      return size * nmemb;
    };
    retCode += curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
    std::string buffer("");
    retCode += curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    if (retCode != CURLE_OK) {
      AV_LOGW(LOG_TAG, "%s curl error\n", __FUNCTION__);
      break;
    }
    retCode = curl_easy_perform(curl);
    AV_LOGI(LOG_TAG, "%s curl_easy_perform return %d\n", __FUNCTION__, retCode);
    if (retCode == CURLE_OK) {
      AV_LOGI(LOG_TAG, "%s buffer: %s\n", __FUNCTION__, buffer.c_str());
      try {
        Json json = Json::parse(buffer);
        if (json["success"]) {
          for (auto &result : json["data"]) {
            std::string domain = result["domain"];
            for (std::string ip : result["ip"]) {
              if (RedDownloadConfig::getinstance()->get_config_value(
                      PARSE_NO_PING) > 0) {
                PingFdInfo fdinfo;
                fdinfo.family = GetIpFamily(ip);
                fdinfo.ipaddr = ip;
                fdinfo.count = 1;
                vector<string> hostnames_temp;
                hostnames_temp.push_back(domain);
                fdinfo.hostnames = hostnames_temp;
                PushAddrInfo(&fdinfo, 0);
              } else {
                PreParePingSingle(domain, ip.c_str(), GetIpFamily(ip));
              }
            }
          }
        } else {
          AV_LOGW(LOG_TAG, "catch json other exception\n");
        }
      } catch (Json::exception &e) {
        AV_LOGW(LOG_TAG, "catch json exception, %s\n", e.what());
      } catch (...) {
        AV_LOGW(LOG_TAG, "catch json other exception\n");
      }
    }
  } while (false);
  if (curl) {
    curl_easy_cleanup(curl);
    curl = nullptr;
  }
  return;
}

void REDDnsCache::PreParePing(string hostname, struct hostent *addrinfo) {
  if (addrinfo == NULL || hostname.empty())
    return;
  char **pptr = addrinfo->h_addr_list;
  for (; *pptr != NULL; pptr++) {
    char ip[96] = {0};
    inet_ntop(addrinfo->h_addrtype, *pptr, ip, sizeof(ip));
    if (strlen(ip) > 0) {
      if (RedDownloadConfig::getinstance()->get_config_value(PARSE_NO_PING) >
          0) {
        PingFdInfo fdinfo;
        fdinfo.family = addrinfo->h_addrtype;
        fdinfo.ipaddr = ip;
        fdinfo.count = 1;
        vector<string> hostnames;
        hostnames.push_back(hostname);
        fdinfo.hostnames = hostnames;
        PushAddrInfo(&fdinfo, 0);
      } else {
        PreParePingSingle(hostname, ip, addrinfo->h_addrtype);
      }
    }
  }
}

void REDDnsCache::PreParePingSingle(string hostname, const char *ip,
                                    int family) {
  auto iter = mfdinfo.begin();
  for (; iter != mfdinfo.end(); ++iter) {
    if (strcmp(iter->ipaddr.c_str(), ip) == 0) {
      iter->hostnames.push_back(hostname);
      return;
    }
  }
  int fd = SetPing(ip, family, 0, 1);
  AV_LOGW(LOG_TAG, "%s, hostname %s, ip %s, fd %d\n", __FUNCTION__,
          hostname.c_str(), ip, fd);
  if (fd > 0) {
    PingFdInfo fdinfo;
    fdinfo.ipaddr = ip;
    fdinfo.hostnames.push_back(hostname);
    fdinfo.family = family;
    fdinfo.fd = fd;
    fdinfo.count = 1;
    struct timeval time_cur;
    gettimeofday(&time_cur, NULL);
    fdinfo.lasttime = time_cur.tv_sec;
    mfdinfo.push_back(fdinfo);
  }
}

void REDDnsCache::RecvPingInfo(int fd, PingFdInfo *fdinfo) {
  if (fdinfo == nullptr || fd <= 0)
    return;
  int seqnum = 0;
  int rtt = RecvPing(fd, fdinfo->ipaddr.c_str(), fdinfo->family, seqnum);
  struct timeval time_cur;
  gettimeofday(&time_cur, NULL);
  time_t tv_sec = time_cur.tv_sec - fdinfo->lasttime;
  int oriseq = fd + MAX_DNSCACHE_SEQUENCE_INTERVAL * fdinfo->count;
  if (tv_sec < MAX_DNSCACHE_PING_TIMEOUT && (rtt == -1 || (seqnum != oriseq))) {
    // AV_LOGW(LOG_TAG, "%s, ip %s rtt %d, seq %d -- %d,  !\n",__FUNCTION__,
    // fdinfo->ipaddr.c_str(), rtt, oriseq, seqnum);
    return;
  } else if (tv_sec >= MAX_DNSCACHE_PING_TIMEOUT &&
             (rtt == -1 || (seqnum != oriseq))) {
    rtt = MAX_DNSCACHE_RTT_DURATION;
    AV_LOGW(LOG_TAG, "%s, ip %s rtt %d, seq %d -- %d,  !\n", __FUNCTION__,
            fdinfo->ipaddr.c_str(), rtt, oriseq, seqnum);
  }

  // AV_LOGW(LOG_TAG, "%s, ip %s rtt %d, count %d!\n",__FUNCTION__,
  // fdinfo->ipaddr.c_str(), rtt, fdinfo->count);
  PushAddrInfo(fdinfo, rtt);
  if (fdinfo->count < MAX_DNSCACHE_PING_COUNT) {
    ++fdinfo->count;
    int newfd = SetPing(fdinfo->ipaddr.c_str(), fdinfo->family, fdinfo->fd,
                        fdinfo->count);
    fdinfo->fd = newfd;
    struct timeval ttime;
    gettimeofday(&ttime, NULL);
    fdinfo->lasttime = ttime.tv_sec;
  } else if (fdinfo->fd > 0) {
    close(fdinfo->fd);
    fdinfo->fd = 0;
  }
}

void REDDnsCache::PushAddrInfo(PingFdInfo *fdinfo, int rtt) {
  std::unique_lock<std::mutex> lock(m_mutex);
  auto iter = mdnscache.begin();
  for (; iter != mdnscache.end(); ++iter) {
    auto iterhost =
        find(fdinfo->hostnames.begin(), fdinfo->hostnames.end(), iter->first);
    if (iterhost == fdinfo->hostnames.end()) {
      continue;
    }
    vector<IpAddrInfo>::iterator ipiter = iter->second->vecipinfo.begin();
    for (; ipiter != iter->second->vecipinfo.end(); ++ipiter) {
      if (ipiter->ipaddr == fdinfo->ipaddr) {
        ipiter->rtt =
            (rtt + ipiter->rtt * (fdinfo->count - 1)) / (fdinfo->count);
        if (rtt < MAX_DNSCACHE_RTT_DURATION && rtt >= 0) {
          ipiter->bvalid = true;
        }
        AV_LOGW(LOG_TAG, "%s, host %s update ip %s rtt %d -> %d, count %d!\n",
                __FUNCTION__, iter->first.c_str(), fdinfo->ipaddr.c_str(), rtt,
                ipiter->rtt, fdinfo->count);
        break;
      }
    }
    if (ipiter == iter->second->vecipinfo.end()) {
      IpAddrInfo ipaddrinfo;
      ipaddrinfo.ipaddr = fdinfo->ipaddr;
      ipaddrinfo.family = fdinfo->family;
      ipaddrinfo.rtt = rtt;
      if (rtt < MAX_DNSCACHE_RTT_DURATION && rtt >= 0) {
        ipaddrinfo.bvalid = true;
      }
      iter->second->vecipinfo.push_back(ipaddrinfo);
      AV_LOGW(LOG_TAG, "%s,host %s new ip %s rtt %d, count %d!\n", __FUNCTION__,
              iter->first.c_str(), fdinfo->ipaddr.c_str(), rtt, fdinfo->count);
    }
  }
}

int REDDnsCache::GetIpFamily(const string &ip) {
  struct sockaddr_in6 sa_6;
  if (inet_pton(AF_INET6, ip.c_str(), &(sa_6.sin6_addr)) != 0) {
    return AF_INET6;
  }
  return AF_INET;
}
