# RedPreload SDK

The RedPreload SDK is a powerful tool for preloading videos and improving the user experience. It provides a range of features, including message callbacks, initialization with cache path and maximum size, and more. This guide will walk you through the steps to use the RedPreload SDK.

## Key Features

- Provides message callbacks for various events
- Supports initialization with cache path and maximum size
- Offers methods to open URLs and [JSON](../../JSON.md) with parameters and user data
- Supports stopping preload and deleting cache

## Header Files References:

* [RedPreLoad.h](../redplayercore/redplayer/ios/RedPreLoad.h)

## ▶️ Getting Started

### Set Preload Message Callback

To set the preload message callback, use the following method:

```objective-c
+ (void)setPreLoadMsgCallback:(RedPreLoadCallback)callback;
```

### Initialize Preload

To initialize preload with cache path and maximum size, use the following method:

```objective-c
+ (void)initPreLoad:(NSString *)cachePath maxSize:(uint32_t)maxSize;
```

To initialize preload with cache path, maximum size(byte), and maximum directory capacity, use the following method:

```objective-c
+ (void)initPreLoad:(NSString *)cachePath maxSize:(uint32_t)maxSize maxdircapacity:(int64_t)maxdircapacity;
```

### Open URLs and JSON

To open URLs with parameters and user data, use the following method:

```objective-c
/// Struct for RedPreloadParam
typedef struct {
    NSString* cachePath; ///< Cache path
    int64_t preloadSize; ///< Preload size bytes
    NSString* referer; ///< Referer
    int dnsTimeout; ///< DNS timeout
    bool useHttps; ///< Use HTTPS or not
    NSString* userAgent; ///< User agent
    NSString* header; ///< Header
    int64_t cache_max_dir_capacity; ///< Maximum directory capacity for cache
    uint32_t cache_max_entries; ///< Maximum entries for cache
} RedPreloadParam;

+ (void)open:(NSURL *)url param:(RedPreloadParam)param userData:(void* _Nullable)userData;
```

To open [JSON](../../JSON.md) with parameters and user data, use the following method:

```objective-c
+ (void)openJson:(NSString *)jsonStr param:(RedPreloadParam)param userData:(void* _Nullable)userData;
```

### Close URLs and JSON

To close URL preload task, use the following method:

```objective-c
+ (void)close:(NSURL *)url;
```

To close JSON preload task, use the following method:

```objective-c
+ (void)closeJson:(NSString *)jsonStr;
```

### Stop Preload

To stop all preload tasks, use the following method:

```objective-c
+ (void)stop;
```

### Get Cached Size and Files

To get cached size, use the following method:

```objective-c
+ (int64_t)getCachedSize:(NSString *)path uri:(NSString*)uri isFullUrl:(BOOL)isFullUrl;
```

To get all cached files, use the following method:

```objective-c
+ (NSArray<NSString *> *)getAllCachedFile:(NSString *)path;
```

### Delete Cache

To delete cache, use the following method:

```objective-c
+ (int)deleteCache:(NSString *)path uri:(NSString *)uri isFullUrl:(BOOL)isFullUrl;
```

