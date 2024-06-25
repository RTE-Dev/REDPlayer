#include "REDPing.h"

#include "RedLog.h"
#define PACKET_SIZE 4096
#define ERROR 0
#define SUCCESS 1

#define LOG_TAG "RedPing"

static unsigned short cal_chksum(unsigned short *addr, int len) {
  int nleft = len;
  int sum = 0;
  unsigned short *w = addr;
  unsigned short answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1) {
    *(unsigned char *)(&answer) = *(unsigned char *)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;

  return answer;
}

static int icmp_pack(char *data, int family, int seq, pid_t pid) {
  if (family == AF_INET) {
    struct icmp *icmpdata = (struct icmp *)data;
    icmpdata->icmp_type = ICMP_ECHO;
    icmpdata->icmp_code = 0;
    icmpdata->icmp_cksum = 0;
    icmpdata->icmp_seq = seq;
    icmpdata->icmp_id = pid & 0xffff;
    struct timeval *tval;
    tval = (struct timeval *)icmpdata->icmp_data;
    gettimeofday(tval, NULL);
    icmpdata->icmp_cksum =
        cal_chksum((unsigned short *)icmpdata, sizeof(struct icmp));
    return sizeof(struct icmp);
  } else if (family == AF_INET6) {
    struct icmp6_hdr icmp6;
    icmp6.icmp6_type = ICMP6_ECHO_REQUEST;
    icmp6.icmp6_code = 0;
    icmp6.icmp6_cksum = 0;
    icmp6.icmp6_id = pid & 0xffff;
    icmp6.icmp6_seq = seq;

    memcpy(data, &icmp6, sizeof(icmp6));
    struct timeval pkt_time;
    gettimeofday(&pkt_time, NULL);
    memcpy(&data[sizeof(icmp6)], &pkt_time, sizeof(pkt_time));
    return sizeof(struct timeval) + sizeof(struct icmp6_hdr);
  }
  return 0;
}

static int icmp_unpack(char *rec_buf, int icmplen, int family, pid_t pid,
                       int &seq) {
  if (family == AF_INET) {
    struct ip *iph = (struct ip *)rec_buf;
    int iphl_len = iph->ip_hl << 2;
    struct icmp *icmp = (struct icmp *)(rec_buf + iphl_len);
    // AV_LOGI(LOG_TAG,"%s, icmp->icmp_type:%d,icmp->icmp_id:%d, seq %d\n",
    // __FUNCTION__, icmp->icmp_type,icmp->icmp_id, icmp->icmp_seq);
    if (icmp->icmp_type == ICMP_ECHOREPLY) {
      seq = icmp->icmp_seq;
      struct timeval time_end;
      gettimeofday(&time_end, NULL);
      struct timeval *begintime = (struct timeval *)icmp->icmp_data;
      time_t tv_sec = time_end.tv_sec - begintime->tv_sec;
      time_t tv_usec = time_end.tv_usec - begintime->tv_usec;
      if (tv_sec < 0) {
        tv_usec += 1000000;
        tv_sec--;
      }
      int64_t rtt = tv_sec * 1000 + tv_usec / 1000;
      // AV_LOGI(LOG_TAG,"%s, icmp ping success, rtt %lld \n",
      // __FUNCTION__, rtt);
      return ((rtt > MAX_DNSCACHE_RTT_DURATION || rtt < 0)
                  ? MAX_DNSCACHE_RTT_DURATION
                  : rtt);
    }
  } else {
    struct icmp6_hdr *icmp6 = (struct icmp6_hdr *)rec_buf;
    // AV_LOGI(LOG_TAG,"%s, icmp6->icmp6_type:%d,icmp6->icmp6_id:%d, seq %d
    // \n", __FUNCTION__, icmp6->icmp6_type,icmp6->icmp6_id,
    // icmp6->icmp6_seq);
    if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
      seq = icmp6->icmp6_seq;
      struct timeval time_end;
      gettimeofday(&time_end, NULL);
      struct timeval begintime;
      memcpy(&begintime, rec_buf + 8, sizeof(begintime));
      time_t tv_sec = time_end.tv_sec - begintime.tv_sec;
      time_t tv_usec = time_end.tv_usec - begintime.tv_usec;
      if (tv_sec < 0) {
        tv_usec += 1000000;
        tv_sec--;
      }
      int64_t rtt = tv_sec * 1000 + tv_usec / 1000;
      // AV_LOGI(LOG_TAG,"%s, icmp6 ping success, rtt %lld \n",
      // __FUNCTION__, rtt);
      return ((rtt > MAX_DNSCACHE_RTT_DURATION || rtt < 0)
                  ? MAX_DNSCACHE_RTT_DURATION
                  : rtt);
    }
  }
  return -1;
}

int RecvPing(int sockfd, const char *ip, int family, int &seqnum) {
  if (sockfd <= 0) {
    return MAX_DNSCACHE_RTT_DURATION;
  }
  struct sockaddr_in from;
  struct sockaddr_in6 from6;
  struct timeval timeo;
  timeo.tv_sec = 5000 / 1000;
  timeo.tv_usec = 5000 % 1000;
  pid_t pid;
  pid = getpid();
  // AV_LOGW(LOG_TAG,"%s, ip %s, pid %d, family %d, fd %d\n", __FUNCTION__,
  // ip, pid, family, sockfd);

  int rtt = MAX_DNSCACHE_RTT_DURATION;
  char rec_buf[PACKET_SIZE];
  int recsize = 0;
  do {
    memset(rec_buf, 0, sizeof(rec_buf));
    char from_ip[96] = {0};
    if (family == AF_INET6) {
      int fromlen = sizeof(from6);
      recsize = recvfrom(sockfd, rec_buf, sizeof(rec_buf), 0,
                         reinterpret_cast<struct sockaddr *>(&from6),
                         reinterpret_cast<socklen_t *>(&fromlen));
      if (recsize < 1) {
        AV_LOGW(LOG_TAG, "%s, rec len %d, ip %s\n", __FUNCTION__, recsize, ip);
        break;
      }
      inet_ntop(AF_INET6, &(from6.sin6_addr), from_ip, sizeof(from_ip));
    } else {
      int fromlen = sizeof(from);
      recsize = recvfrom(sockfd, rec_buf, sizeof(rec_buf), 0,
                         reinterpret_cast<struct sockaddr *>(&from),
                         reinterpret_cast<socklen_t *>(&fromlen));
      if (recsize < 1) {
        AV_LOGW(LOG_TAG, "%s, rec len %d, ip %s\n", __FUNCTION__, recsize, ip);
        break;
      }
      inet_ntop(AF_INET, &(from.sin_addr), from_ip, sizeof(from_ip));
    }

    if (strcmp(from_ip, ip) != 0) {
      AV_LOGW(LOG_TAG, "%s, diff ip %s from ip %s\n", __FUNCTION__, ip,
              from_ip);
      return -1;
    }
    rtt = icmp_unpack(rec_buf, recsize, family, pid, seqnum);
  } while (0);
  // AV_LOGW(LOG_TAG,"%s, rtt %d, ip %s, size %d\n", __FUNCTION__, rtt, newip,
  // recsize);
  return rtt;
}

int SetPing(const char *ip, int family, int fd, int count) {
  struct sockaddr_in addr;
  struct sockaddr_in6 addr6;
  if (family == AF_INET6) {
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip, &(addr6.sin6_addr));
  } else {
    bzero(&addr, sizeof(addr));
    addr.sin_family = family;
    addr.sin_addr.s_addr = inet_addr(ip);
  }
  struct timeval timeo;
  timeo.tv_sec = MAX_DNSCACHE_PING_TIMEOUT;
  timeo.tv_usec = 0;
  pid_t pid;
  pid = getpid();
  // AV_LOGW(LOG_TAG,"%s, pid %d, family %d, ip %s\n", __FUNCTION__, pid,
  // family, ip);
  int sockfd = 0;
  do {
    if (fd > 0) {
      close(fd);
    }
    if (family == AF_INET6) {
      sockfd = socket(family, SOCK_DGRAM, IPPROTO_ICMPV6);
    } else {
      sockfd = socket(family, SOCK_DGRAM, IPPROTO_ICMP);
    }
    if (sockfd < 0 || sockfd > 1023) {
      AV_LOGW(LOG_TAG, "%s, get socket err %d, ip %s\n", __FUNCTION__, sockfd,
              ip);
      goto fail;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) ==
        -1) {
      AV_LOGW(LOG_TAG, "%s, set socket send err, fd %d, ip %s\n", __FUNCTION__,
              sockfd, ip);
      goto fail;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo)) ==
        -1) {
      AV_LOGW(LOG_TAG, "%s, set socket rec err, fd %d, ip %s\n", __FUNCTION__,
              sockfd, ip);
      goto fail;
    }
    char send_buf[PACKET_SIZE];
    int rcmp_seq = sockfd + MAX_DNSCACHE_SEQUENCE_INTERVAL * count;
    memset(send_buf, 0, sizeof(send_buf));
    int datalen = icmp_pack(send_buf, family, rcmp_seq, pid);
    int sendsize = 0;
    if (family == AF_INET6) {
      sendsize =
          sendto(sockfd, reinterpret_cast<char *>(&send_buf), datalen, 0,
                 reinterpret_cast<struct sockaddr *>(&addr6), sizeof(addr6));
    } else {
      sendsize =
          sendto(sockfd, reinterpret_cast<char *>(&send_buf), datalen, 0,
                 reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    }
    if (sendsize < 1) {
      AV_LOGW(LOG_TAG, "%s, sendto err, size %d, ip %s\n", __FUNCTION__,
              sendsize, ip);
      goto fail;
    }
  } while (0);
  AV_LOGW(LOG_TAG, "%s, pid %d, family %d, ip %s, fd %d\n", __FUNCTION__, pid,
          family, ip, sockfd);
  return sockfd;
fail:
  if (sockfd > 0) {
    close(sockfd);
    sockfd = 0;
  }
  return 0;
}
