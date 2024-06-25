#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_DNSCACHE_RTT_DURATION 100000
#define MAX_DNSCACHE_SEQUENCE_INTERVAL 1024
#define MAX_DNSCACHE_PING_COUNT 5
#define MAX_DNSCACHE_PING_TIMEOUT 5
#define MAX_ARES_PARSE_TIMEOUT 100
int RecvPing(int fd, const char *ip, int family, int &seqnum);
int SetPing(const char *ipaddr, int family, int sockfd, int count);
