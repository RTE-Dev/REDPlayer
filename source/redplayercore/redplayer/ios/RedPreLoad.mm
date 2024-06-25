//
//  RedPreLoad.m
//  XYRedPlayer
//
//  Created by 张继东 on 2023/12/19.
//  Copyright © 2023 XingIn. All rights reserved.
//

#import "RedPreLoad.h"

#include "strategy/RedAdaptiveStrategy.h"
#include "wrapper/reddownload_datasource_wrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/log.h"
#ifdef __cplusplus
}
#endif

@interface RedPreLoad ()
@property(nonatomic, copy) RedPreLoadCallback preloadCallback;
@property(nonatomic, strong) NSLock *callbackLock;
@property(nonatomic, strong) dispatch_queue_t serialQueue;
@property(nonatomic, copy) NSString *cacheUrlString;

@end

@implementation RedPreLoad

#pragma mark - life cycle
+ (instancetype)shared {
  static RedPreLoad *_instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[self alloc] init];
  });
  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _callbackLock = [NSLock new];
    _serialQueue = dispatch_queue_create("com.xiaohongshu.red.preload.queue",
                                         DISPATCH_QUEUE_SERIAL);
  }
  return self;
}

- (void)dealloc {
}
#pragma mark - instance methods
- (void)openJson:(NSString *)jsonStr
           param:(RedPreloadParam)param
        userData:(void *)userData {
  if (jsonStr == nil || param.cachePath == nil) {
    av_log(NULL, AV_LOG_ERROR, "preload openJson param is nil\n");
    return;
  }
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    int ret = -1;
    std::string url;
    DownLoadOptWrapper opt;
    reddownload_datasource_wrapper_opt_reset(&opt);
    opt.cache_file_dir = param.cachePath.UTF8String;
    opt.PreDownLoadSize = param.preloadSize;
    if (param.referer != nil)
      opt.referer = param.referer.UTF8String;
    if (param.dnsTimeout > 0)
      opt.dns_timeout = param.dnsTimeout;
    opt.use_https = param.useHttps;

    if (param.userAgent) {
      opt.useragent = param.userAgent.UTF8String;
    }
    if (param.header) {
      opt.headers = param.header.UTF8String;
    }
    std::unique_ptr<redstrategycenter::strategy::RedAdaptiveStrategy> strategy =
        nullptr;
    strategy =
        std::make_unique<redstrategycenter::strategy::RedAdaptiveStrategy>(
            redstrategycenter::adaptive::logic::AbstractAdaptationLogic::
                LogicType::Adaptive);
    if (strategy) {
      strategy->setPlaylist(url);
      strategy->setReddownloadUrlCachedFunc(
          reddownload_datasource_wrapper_cache_size);
      url = strategy->getInitialUrl(strategy->getInitialRepresentation());
    }
    if (url.empty()) {
      av_log(NULL, AV_LOG_ERROR, "preload open json %s failed",
             jsonStr.UTF8String);
    }
    self.cacheUrlString = [NSString stringWithCString:url.c_str()
                                             encoding:NSUTF8StringEncoding];
    av_log(NULL, AV_LOG_INFO, "preload open url %s\n", url.c_str());
    DownLoadCbWrapper wrapper = {0};
    wrapper.appctx = userData;
    wrapper.ioappctx = NULL;
    wrapper.appcb = appcb;
    wrapper.ioappcb = NULL;
    wrapper.urlcb = NULL;
    wrapper.speedcb = speedcb;
    reddownload_datasource_wrapper_open(url.c_str(), &wrapper, &opt);
  });
}

- (void)open:(NSURL *)url
       param:(RedPreloadParam)param
    userData:(void *)userData {
  if (url == nil || param.cachePath == nil) {
    av_log(NULL, AV_LOG_ERROR, "preload open param is nil\n");
    return;
  }
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    NSString *urlString = url.isFileURL ? url.path : url.absoluteString;
    self.cacheUrlString = urlString;
    av_log(NULL, AV_LOG_INFO, "preload open %" PRId64 " %s\n",
           param.preloadSize, urlString.UTF8String);
    DownLoadOptWrapper opt;
    reddownload_datasource_wrapper_opt_reset(&opt);
    opt.cache_file_dir = param.cachePath.UTF8String;
    opt.PreDownLoadSize = param.preloadSize;
    if (param.referer != nil && param.referer.length > 0)
      opt.referer = param.referer.UTF8String;
    if (param.dnsTimeout > 0)
      opt.dns_timeout = param.dnsTimeout;
    opt.use_https = param.useHttps;
    if (param.userAgent && param.userAgent.length > 0) {
      opt.useragent = param.userAgent.UTF8String;
    }
    if (param.header && param.header.length > 0) {
      opt.headers = param.header.UTF8String;
    }
    if (param.cache_max_dir_capacity > 0) {
      opt.cache_max_dir_capacity = param.cache_max_dir_capacity;
    }
    if (param.cache_max_entries > 0) {
      opt.cache_max_entries = opt.cache_max_entries;
    }
    DownLoadCbWrapper wrapper = {0};
    wrapper.appctx = userData;
    wrapper.ioappctx = NULL;
    wrapper.appcb = appcb;
    wrapper.ioappcb = NULL;
    wrapper.urlcb = NULL;
    wrapper.speedcb = speedcb;
    reddownload_datasource_wrapper_open(urlString.UTF8String, &wrapper, &opt);
  });
}

- (void)close {
  if (self.cacheUrlString == nil) {
    return;
  }
  NSString *urlString = self.cacheUrlString;
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    av_log(NULL, AV_LOG_INFO, "preload release %s\n", urlString.UTF8String);
    reddownload_datasource_wrapper_close(urlString.UTF8String, 0);
  });
}

#pragma mark - class method
+ (void)setPreLoadMsgCallback:(RedPreLoadCallback)callback {
  [[RedPreLoad shared].callbackLock lock];
  [RedPreLoad shared].preloadCallback = callback;
  [[RedPreLoad shared].callbackLock unlock];
}

+ (void)initPreLoad:(NSString *)cachePath maxSize:(uint32_t)maxSize {
  if (cachePath == nil) {
    av_log(NULL, AV_LOG_ERROR, "initPreLoad cachePath is nil\n");
    return;
  }
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    av_log(NULL, AV_LOG_INFO, "initPreLoad %s %u\n", cachePath.UTF8String,
           maxSize);
    DownLoadOptWrapper opt;
    reddownload_datasource_wrapper_opt_reset(&opt);
    opt.cache_file_dir = cachePath.UTF8String;
    opt.cache_max_entries = maxSize;
    reddownload_datasource_wrapper_init(&opt);
  });
}

+ (void)initPreLoad:(NSString *)cachePath
            maxSize:(uint32_t)maxSize
     maxdircapacity:(int64_t)maxdircapacity {
  if (cachePath == nil) {
    av_log(NULL, AV_LOG_ERROR, "initPreLoad cachePath is nil\n");
    return;
  }
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    av_log(NULL, AV_LOG_INFO, "initPreLoad %s %u\n", cachePath.UTF8String,
           maxSize);
    DownLoadOptWrapper opt;
    reddownload_datasource_wrapper_opt_reset(&opt);
    opt.cache_file_dir = cachePath.UTF8String;
    opt.cache_max_entries = maxSize;
    if (maxdircapacity > 0)
      opt.cache_max_dir_capacity = maxdircapacity;
    reddownload_datasource_wrapper_init(&opt);
  });
}

+ (void)readDns:(NSURL *)url param:(RedPreloadParam)param {
  if (url == nil) {
    av_log(NULL, AV_LOG_ERROR, "preload read_dns url is nil\n");
    return;
  }
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    NSString *urlString = url.isFileURL ? url.path : url.absoluteString;
    av_log(NULL, AV_LOG_INFO, "preload read_dns %s\n", urlString.UTF8String);
    DownLoadOptWrapper opt;
    reddownload_datasource_wrapper_opt_reset(&opt);
    opt.DownLoadType = 1; // DOWNLOADDNS
    if (param.dnsTimeout > 0)
      opt.dns_timeout = param.dnsTimeout;
    if (param.referer)
      opt.referer = param.referer.UTF8String;
    DownLoadCbWrapper wrapper = {0};
    wrapper.appctx = NULL;
    wrapper.ioappctx = NULL;
    wrapper.appcb = appcb;
    wrapper.ioappcb = NULL;
    wrapper.urlcb = NULL;
    wrapper.speedcb = NULL;
    reddownload_datasource_wrapper_open(urlString.UTF8String, &wrapper, &opt);
  });
}

+ (void)stop {
  dispatch_async([RedPreLoad shared].serialQueue, ^{
    av_log(NULL, AV_LOG_INFO, "preload stop begin\n");
    reddownload_datasource_wrapper_stop(NULL, 0);
    av_log(NULL, AV_LOG_INFO, "preload stop end\n");
  });
}

+ (BOOL)isPreloadUrlHaveCached:(NSString *)url {
  if (!url)
    return false;
  int64_t cache_size =
      reddownload_datasource_wrapper_cache_size(url.UTF8String);
  av_log(NULL, AV_LOG_INFO, "preload have cache size:%" PRId64 " url: %s\n",
         cache_size, url.UTF8String);
  return cache_size > 0;
}

+ (int64_t)getCachedSize:(NSString *)path
                     uri:(NSString *)uri
               isFullUrl:(BOOL)isFullUrl {
  if (!path || !uri)
    return -1;
  int64_t cache_size = reddownload_datasource_wrapper_cache_size_by_uri(
      path.UTF8String, uri.UTF8String, isFullUrl);
  av_log(NULL, AV_LOG_INFO,
         "preload get cache size:%" PRId64 " path: %s url: %s\n", cache_size,
         path.UTF8String, uri.UTF8String);
  return cache_size;
}

+ (NSArray<NSString *> *)getAllCachedFile:(NSString *)path {
  if (!path)
    return NULL;

  NSMutableArray<NSString *> *result = [NSMutableArray array];

  char **cached_file;
  int cached_file_len = 0;
  reddownload_get_all_cached_file(path.UTF8String, &cached_file,
                                  &cached_file_len);
  if (cached_file_len <= 0) {
    return NULL;
  }

  for (int i = 0; i < cached_file_len; i++) {
    NSString *file = [NSString stringWithUTF8String:*(cached_file + i)];
    [result addObject:file];
    free(*(cached_file + i));
  }

  av_log(NULL, AV_LOG_INFO, "preload cache path: %s, file_len: %d\n",
         path.UTF8String, cached_file_len);
  return result;
}

+ (int)deleteCache:(NSString *)path
               uri:(NSString *)uri
         isFullUrl:(BOOL)isFullUrl {
  if (!path || !uri)
    return -1;
  reddownload_delete_cache(path.UTF8String, uri.UTF8String, isFullUrl);
  av_log(NULL, AV_LOG_INFO, "preload delete cache path: %s， uri: %s\n",
         path.UTF8String, uri.UTF8String);
  return 0;
}

+ (NSString *)getCacheFilePath:(NSString *)cachePath url:(NSString *)url {
  char *cache_file_path;
  int ret = reddownload_get_cache_file_path(cachePath.UTF8String,
                                            url.UTF8String, &cache_file_path);
  NSString *ret_path = [NSString stringWithUTF8String:cache_file_path];
  free(cache_file_path);
  return ret_path;
}

+ (NSString *)getUrlMd5Path:(NSString *)url {
  char outMd5path[1024];
  int ret = reddownload_get_url_md5(url.UTF8String, outMd5path, 1023);
  NSString *ret_path = [NSString stringWithUTF8String:outMd5path];
  return ret_path;
}

#pragma mark - c callback
static void appcb(void *app_ctx, int msg, void *arg1, void *arg2) {
  [[RedPreLoad shared].callbackLock lock];
  RedPreLoadCallback block = [RedPreLoad shared].preloadCallback;
  if (block == nil) {
    [[RedPreLoad shared].callbackLock unlock];
    return;
  }
  switch (msg) {
  case RED_EVENT_DID_FRAGMENT_COMPLETE_W: {
    if (!arg1)
      break;
    RedIOTrafficWrapper *traffic = (RedIOTrafficWrapper *)arg1;
    RedPreLoadTask task = {0};
    task.error = 0;
    task.url = [NSString stringWithCString:traffic->url
                                  encoding:NSUTF8StringEncoding];
    task.trafficBytes = traffic->bytes;
    task.tcpSpeed = 0;
    task.cacheFilePath = [NSString stringWithUTF8String:traffic->cache_path];
    task.cacheSize = traffic->cache_size;
    block(task, RedPreLoadControllerMsgTypeCompleted, app_ctx);
    break;
  }
  case RED_EVENT_WILL_DNS_PARSE_W: {
    if (!arg1)
      break;
    RedDnsInfoWrapper *info = (RedDnsInfoWrapper *)arg1;
    RedPreLoadTask task = {0};
    task.error = 0;
    task.host = [NSString stringWithUTF8String:info->domain];
    block(task, RedPreLoadControllerMsgTypeDnsWillParse, app_ctx);
    break;
  }
  case RED_EVENT_DID_DNS_PARSE_W: {
    if (!arg1)
      break;
    RedDnsInfoWrapper *info = (RedDnsInfoWrapper *)arg1;
    RedPreLoadTask task = {0};
    task.error = info->status;
    task.host = [NSString stringWithUTF8String:info->domain];
    block(task, RedPreLoadControllerMsgTypeDnsDidParse, app_ctx);
    break;
  }
  case RED_EVENT_DID_HTTP_OPEN_W: {
    if (!arg1)
      break;
    RedHttpEventWrapper *httpEvent = (RedHttpEventWrapper *)arg1;
    if (httpEvent->error != 0) {
      RedPreLoadTask task = {0};
      task.error = httpEvent->error;
      task.url = [NSString stringWithCString:httpEvent->url
                                    encoding:NSUTF8StringEncoding];
      task.tcpSpeed = 0;
      task.trafficBytes = 0;
      block(task, RedPreLoadControllerMsgTypeError, app_ctx);
    }
    break;
  }
  case RED_CTRL_DID_TCP_OPEN_W: {
    if (!arg1)
      break;
    RedTcpIOControlWrapper *tcpIO = (RedTcpIOControlWrapper *)arg1;
    if (tcpIO->error != 0) {
      RedPreLoadTask task = {0};
      task.error = tcpIO->error;
      task.url = [NSString stringWithCString:tcpIO->url
                                    encoding:NSUTF8StringEncoding];
      task.tcpSpeed = 0;
      task.trafficBytes = 0;
      block(task, RedPreLoadControllerMsgTypeError, app_ctx);
    }
    break;
  }
  case RED_EVENT_DID_RELEASE_W: {
    RedPreLoadTask task = {0};
    block(task, RedPreLoadControllerMsgTypeRelease, app_ctx);
    break;
  }
  default:
    break;
  }
  [[RedPreLoad shared].callbackLock unlock];
}

static void speedcb(void *opaque, int64_t size, int64_t speed,
                    int64_t cur_time) {
  redstrategycenter::RedStrategyCenter::GetInstance()->updateDownloadRate(
      size, speed, cur_time);

  [[RedPreLoad shared].callbackLock lock];
  RedPreLoadCallback block = [RedPreLoad shared].preloadCallback;
  if (block == nil) {
    [[RedPreLoad shared].callbackLock unlock];
    return;
  }
  RedPreLoadTask task;
  task.error = 0;
  task.tcpSpeed = speed;
  task.url = nil;
  task.trafficBytes = 0;
  block(task, RedPreLoadControllerMsgTypeSpeed, opaque);
  [[RedPreLoad shared].callbackLock unlock];
}

@end
