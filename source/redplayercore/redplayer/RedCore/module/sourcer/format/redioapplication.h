#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CACHE_FILE_PATH_MAX_LEN 512
#define MD5_CHAR_MAX_LEN 256
#define IP_CHAR_MAX_LEN 46

#define REDIOAPP_EVENT_CACHE_STATISTIC 0x1003
#define REDIOAPP_EVENT_CRONET_NETWORK_STATISTIC 0x1004
#define REDIOAPP_EVENT_DEFAULT_TCP_RECV_BUFFER 0x1005
#define REDIOAPP_EVENT_ERROR 0x1006
#define REDIOAPP_EVENT_PCDN_SOURCE 0x1007
#define REDIOAPP_EVENT_PCDN_SWITCH_INFO 0x1008
#define REDIOAPP_EVENT_FIRST_CATON_OFFSET 0x1009
#define REDIOAPP_EVENT_CONTAIN_PCDN_STREAM 0x1010

typedef struct RedApplicationContext RedApplicationContext;
struct RedApplicationContext {
  void *opaque; /**< user data. */
  int (*func_on_app_event)(RedApplicationContext *h, int event_type, void *obj);
};

typedef struct RedIOAppCacheStatistic {
  int64_t cache_physical_pos;
  int64_t cache_file_forwards;
  int64_t cache_file_pos;
  int64_t cache_count_bytes;
  int64_t logical_file_size;
  int64_t cached_size;
} RedIOAppCacheStatistic;

typedef struct RedIOAppNetworkStatistic {
  int64_t dns_cost;
  int64_t connect_cost;
  int64_t request_cost;
  int64_t ssl_cost;
  char remote_ip[IP_CHAR_MAX_LEN];
} RedIOAppNetworkStatistic;

typedef struct RedCacheEntry {
  int64_t logical_pos;
  int64_t physical_pos;
  int64_t size;
} RedCacheEntry;

typedef struct RedIOAppErrorEvent {
  char url[4096];
  int error;
  int source;
} RedIOAppErrorEvent;
