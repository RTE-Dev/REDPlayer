#pragma once

#include <stdio.h>
#ifdef __APPLE__
#include <resolv.h>
#endif
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include <ares.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef __cplusplus
}
#endif
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "REDDownloadListen.h"
#include "REDPing.h"
using namespace std;
struct UserData {
  string hostname;
  void *cacheptr;
  UserData() : cacheptr(nullptr) {}
  UserData(string cname, void *cptr) : hostname(cname), cacheptr(cptr) {}
};

class REDDnsCache {
public:
  static REDDnsCache *getinstance();
  ~REDDnsCache();
  void NetWorkChanged();
  void SetHostDnsServerIp(string dnsserver);
  void SetDnsCb(DownLoadListen *cb);
  void AddHost(string hostname);
  void SetTimescale(int timescale);
  void UpdateAddr(string hostname, string curip, int64_t speed, int err);
  void run();
  string GetIpAddr(string hostname);
  string GetValidHost();

private:
  REDDnsCache();
  static REDDnsCache *minstance;
  static void Cares_Cb(void *userdata, int status, int timeout,
                       struct hostent *addrinfo);
  bool babort;
  int mtimescale; // ms
  std::thread *mthread = nullptr;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  struct IpAddrInfo {
    string ipaddr;
    int family{AF_INET};
    int64_t successcounts{1};
    int64_t usecounts{1};
    int64_t speed{0};
    bool bvalid{false};
    int rtt{MAX_DNSCACHE_RTT_DURATION};
  };
  struct HostData {
    UserData *userdata{nullptr};
    vector<IpAddrInfo> vecipinfo;
  };
  struct PingFdInfo {
    string ipaddr;
    vector<string> hostnames;
    int family{AF_INET};
    int fd{0};
    int count{0};
    time_t lasttime{0};
  };
  vector<PingFdInfo> mfdinfo;
  unordered_map<string, HostData *> mdnscache;
  string mvalidhost;
  string mdnsderverip;
  DownLoadListen *mdownloadcb;
  int mhttpdns{0};
  const char *httpdnsServer = "https://gslb.xiaohongshu.com/domain?domains=";

private:
  static const ares_socket_functions default_functions;
  void RunLoop(string threadname);
  void ParseDns();
  void ParseDnsLocal(string hostname);
  void ParseDnsCares(vector<string> &hostnames);
  void GetHttpDns(vector<string> &hostnames);
  void WaitHttpDnsPing();
  void ClearDnsCache();
  void ClearFdInfo();
  void RecvPingInfo(int fd, PingFdInfo *fdinfo);
  void PushAddrInfo(PingFdInfo *fdinfo, int rtt);
  void PreParePing(string hostname, struct hostent *addrinfo);
  void PreParePingSingle(string hostname, const char *ip, int family);
  string GetHostDnsServerIp();
  void ResetPing();
  int Add_fds(fd_set *readers, fd_set *writers);
  void Get_fds(fd_set *readers, fd_set *writers);
  int GetIpFamily(const string &ip);
};
