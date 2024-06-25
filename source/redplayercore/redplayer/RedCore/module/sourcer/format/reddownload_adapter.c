
#include "RedCore/module/sourcer/format/redioapplication.h"
#include "libavformat/url.h"
#include "libavutil/avstring.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "wrapper/reddownload_datasource_wrapper.h"
#include <stdbool.h>

typedef struct Context {
  AVClass *class;
  char url[4096];
  int64_t uid;
  char *cache_file_dir;
  int64_t pre_download_size;
  int64_t cache_max_dir_capacity;
  int ip_type;
  int64_t dns_cache_timeout;
  char *headers;
  char *useragent;
  char *http_proxy;
  char *referer;
  int max_retry;
  int low_speed_limit; // 0-1
  int low_speed_time;  // ms
  int connect_timeout; // ms
  int readasync;
  int loadfile;
  int tcp_buffer;
  int dns_timeout;
  int enable_url_list_fallback;
  int use_https;

  // app
  char *app_ctx_intptr;
  RedApplicationContext *app_ctx;
} Context;

static void speedcallback(void *opaque, int64_t size, int64_t speed,
                          int64_t cur_time) {}

static void callbackwrapper(void *h, int val, void *a1, void *a2) {
  RedApplicationContext *app_ctx = (RedApplicationContext *)h;
  if (app_ctx && app_ctx->func_on_app_event) {
    switch (val) {
    case RED_CTRL_WILL_TCP_OPEN_W: {
      RedTcpIOControlWrapper data = {};
      app_ctx->func_on_app_event(app_ctx, val, (void *)&data);
      if (a1) {
        int *arg1_w = (int *)malloc(sizeof(int));
        if (!arg1_w) {
          return;
        }
        memcpy(arg1_w, a1, sizeof(int));
        app_ctx->func_on_app_event(
            app_ctx, REDIOAPP_EVENT_DEFAULT_TCP_RECV_BUFFER, arg1_w);
        free(arg1_w);
      }
      break;
    }
    case RED_EVENT_WILL_HTTP_OPEN_W: {
      RedHttpEventWrapper data = {};
      app_ctx->func_on_app_event(app_ctx, val, &data);
      break;
    }
    case RED_EVENT_DID_FRAGMENT_COMPLETE_W: {
      int arg1_w = 0; // func_on_app_event will check if the third param is NULL
      app_ctx->func_on_app_event(app_ctx, val, &arg1_w);
      break;
    }
    default:
      app_ctx->func_on_app_event(app_ctx, val, a1);
      break;
    }
  }
}

static void context_to_optwrapper(Context *c, DownLoadOptWrapper *opt) {
  if (!c || !opt) {
    return;
  }
  if (c->pre_download_size > 0) {
    opt->PreDownLoadSize = c->pre_download_size;
  }
  if (c->cache_file_dir) {
    opt->cache_file_dir = c->cache_file_dir;
  }
  if (c->cache_max_dir_capacity > 0) {
    opt->cache_max_dir_capacity = c->cache_max_dir_capacity;
  }
  if (c->ip_type >= 0) {
    opt->iptype = c->ip_type;
  }
  if (c->dns_cache_timeout > 0) {
    opt->dns_cache_timeout = c->dns_cache_timeout;
  }
  if (c->headers) {
    opt->headers = c->headers;
  }
  if (c->useragent) {
    opt->useragent = c->useragent;
  }
  if (c->http_proxy) {
    opt->http_proxy = c->http_proxy;
  }
  if (c->referer) {
    opt->referer = c->referer;
  }
  if (c->max_retry > 0) {
    opt->max_retry = c->max_retry;
  }
  if (c->low_speed_limit >= 0) {
    opt->low_speed_limit = c->low_speed_limit;
  }
  if (c->low_speed_time > 0) {
    opt->low_speed_time = c->low_speed_time;
  }
  if (c->connect_timeout > 0) {
    opt->connect_timeout = c->connect_timeout;
  }
  opt->readasync = c->readasync;
  opt->loadfile = c->loadfile;
  if (c->dns_timeout > 0) {
    opt->dns_timeout = c->dns_timeout;
  }
  if (c->enable_url_list_fallback) {
    opt->url_list_separator = ";";
  }
  if (c->tcp_buffer > 0) {
    opt->tcp_buffer = c->tcp_buffer;
  }
  opt->use_https = c->use_https;
}

static int reddownload_adapter_open(URLContext *h, const char *arg, int flags,
                                    AVDictionary **options) {
  Context *c = h->priv_data;
  c->uid = 0;
  c->app_ctx = (RedApplicationContext *)av_dict_strtoptr(c->app_ctx_intptr);
  av_strstart(arg, "httpreddownload:", &arg);
  memcpy(c->url, arg, strlen(arg));
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  context_to_optwrapper(c, &opt);
  DownLoadCbWrapper wrapper = {0};
  wrapper.appctx = c->app_ctx;
  wrapper.appcb = callbackwrapper;
  wrapper.speedcb = speedcallback;
  int64_t res = reddownload_datasource_wrapper_open(arg, &wrapper, &opt);

  if (res < 0) {
    av_log(NULL, AV_LOG_INFO, "[%s] open failed!\n", __func__);
    return -1;
  }
  c->uid = res;
  return 0;
}

static int reddownload_adapter_close(URLContext *h) {
  Context *c = h->priv_data;
  int ret = reddownload_datasource_wrapper_close(c->url, c->uid);
  return ret;
}

static int reddownload_adapter_read(URLContext *h, unsigned char *buf,
                                    int size) {
  Context *c = h->priv_data;
  int ret = reddownload_datasource_wrapper_read(c->url, c->uid, buf, size);
  return ret;
}

static int64_t reddownload_adapter_seek(URLContext *h, int64_t pos,
                                        int whence) {
  Context *c = h->priv_data;
  int64_t ret =
      reddownload_datasource_wrapper_seek(c->url, c->uid, pos, whence);
  return ret;
}

#define OFFSET(x) offsetof(Context, x)
#define D AV_OPT_FLAG_DECODING_PARAM

static const AVOption options[] = {
    {"cache_file_dir",
     "cache file dir",
     OFFSET(cache_file_dir),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     D},
    {"pre_download_size",
     "pre download size",
     OFFSET(pre_download_size),
     AV_OPT_TYPE_INT64,
     {.i64 = -1},
     -1,
     (double)INT64_MAX,
     D},
    {"cache_max_dir_capacity",
     "cache max dir capacity",
     OFFSET(cache_max_dir_capacity),
     AV_OPT_TYPE_INT64,
     {.i64 = -1},
     -1,
     (double)INT64_MAX,
     D},
    {"ip_type",
     "ip type",
     OFFSET(ip_type),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     .flags = D},
    {"dns_cache_timeout",
     "dns cache TTL (in microseconds)",
     OFFSET(dns_cache_timeout),
     AV_OPT_TYPE_INT64,
     {.i64 = 0},
     0,
     (double)INT64_MAX,
     D},
    {"headers",
     "headers",
     OFFSET(headers),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     D},
    {"useragent",
     "useragent",
     OFFSET(useragent),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     D},
    {"max_retry",
     "max retry",
     OFFSET(max_retry),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     .flags = D},
    {"low_speed_limit",
     "low speed limit",
     OFFSET(low_speed_limit),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     .flags = D},
    {"low_speed_time",
     "low speed time",
     OFFSET(low_speed_time),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     .flags = D},
    {"connect_timeout",
     "connect timeout",
     OFFSET(connect_timeout),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     .flags = D},
    {"read_async",
     "if read async",
     OFFSET(readasync),
     AV_OPT_TYPE_INT,
     {.i64 = 0},
     0,
     1,
     D},
    {"load_file",
     "if load to file",
     OFFSET(loadfile),
     AV_OPT_TYPE_INT,
     {.i64 = 1},
     0,
     1,
     D},
    {"redapplication",
     "RedApplicationContext",
     OFFSET(app_ctx_intptr),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     .flags = D},
    {"http_proxy",
     "set HTTP proxy to tunnel through",
     OFFSET(http_proxy),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     D},
    {"referer",
     "override referer header",
     OFFSET(referer),
     AV_OPT_TYPE_STRING,
     {.str = NULL},
     0,
     0,
     D},
    {"dns_timeout",
     "dns timeout",
     OFFSET(dns_timeout),
     AV_OPT_TYPE_INT,
     {.i64 = 0},
     0,
     INT_MAX,
     D},
    {"enable-url-list-fallback",
     "enable using the url list to retry network error",
     OFFSET(enable_url_list_fallback),
     AV_OPT_TYPE_INT,
     {.i64 = 0},
     0,
     1,
     D},
    {"tcp_buffer",
     "socket option -- SO_RCVBUF (bytes)",
     OFFSET(tcp_buffer),
     AV_OPT_TYPE_INT,
     {.i64 = -1},
     -1,
     INT_MAX,
     D},
    {"use_https",
     "replace http with https",
     OFFSET(use_https),
     AV_OPT_TYPE_INT,
     {.i64 = 0},
     0,
     1,
     D},

    {NULL}};

#undef D
#undef OFFSET

static const AVClass reddownload_adapter_context_class = {
    .class_name = "ReddownloadAdatper",
    .item_name = av_default_item_name,
    .option = options,
    .version = LIBAVUTIL_VERSION_INT,
};

URLProtocol redmp_ff_reddownload_adapter_protocol = {
    .name = "httpreddownload",
    .url_open2 = reddownload_adapter_open,
    .url_read = reddownload_adapter_read,
    .url_seek = reddownload_adapter_seek,
    .url_close = reddownload_adapter_close,
    .priv_data_size = sizeof(Context),
    .priv_data_class = &reddownload_adapter_context_class,
};
