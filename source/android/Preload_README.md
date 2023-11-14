# How to use the Android RedPreload SDK
This guide provides step-by-step instructions on how to initialize and configure the Android SDK for our open-source video preload. When use preload to download video data, your MediaPlayer can start more faster than normal. So we highly recommand you combine the RedPreload SDK and [RedPlayer SDK](README.md) to usage.

## Key Features
- Supports preload videos in both normal URL and [JSON](../../JSON.md) formats
- Provides options to configure the preload, such as download size, download cache dir
- Supports monitoring preload status and callbacks for various events

## API References:
* [IMediaPreload.java](OpenRedPreload/src/main/java/com/xingin/openredpreload/api/IMediaPreload.kt)


## ▶️ Getting Started

## Step 1: Create media preload
```java
IMediaPreoad mMediaPreload =  new RedMediaPreload();
```

## Step 2: Create preload request
```java
pubic class VideoCacheRequest {
    public String name; // preload task name
    public String videoUrl; // preload resource url
    public String videoJson; // preload resource json url, high than videoUrl
    public String cacheDir; // preload cache dir
    public String cacheSize; //  preload data size 
    public String cacheDirMaxSize; // preload dir max size: when beyond, start LRU
    public String cacheDirMaxEntries; // preload dir max file count: when beyond,start LRU
}

preloadRequest = new VideoCacheRequest()
```

## Step 3: Start preload request
```java
mMediaPreload.start(preloadRequest)
```
By following the three steps above, you can complete the use of the Media Preload SDK 


## Step 4: Common preload methods
```java
void start(VideCacheRequest request); // start preload
        
void stop(); // stop preload
        
void release(); // release preload module: can't start again, need recreate
        
long getAllCacheFilePaths(String cacheDir); //  get the dir's all files
        
long deleteCacheFile(String cacheDir, String url); // delete the cache dir task: the url is the key of preload task
        
float getCacheFileRealSize(String cacheDir, String url); // get the file real download size: not the file size
```

## Step 5: Preload status monitoring & callbacks
- **PreloadCacheListener** This interface runs through the entire lifecycle of the preload, it will be called with many different event.
```java
interface PreloadCacheListener {

    /**
     * @param cacheFilePath file absolute path
     * @param cacheFileSize file size, unit: b
     * @param request origin preload request
     */
    void onPreloadSuccess(String cacheFilePath, long cacheFileSize, VideoCacheRequest request);

    /**
     * @param errorMsg preload error msg
     * @param request origin preload request
     */
    void onPreloadError(long errorMsg, VideoCacheRequest request);

    /**
     * @param request origin preload request
     */
    void onPreloadStart(VideoCacheRequest request);
}

void addPreloadCacheListener(PreloadCacheListener listener); // add the preload status listener

void removePreloadCacheListener(PreloadCacheListener listener); // remove the preload status listener
```

