package com.xingin.openredpreload.api

import com.xingin.openredpreload.impl.model.VideoCacheRequest

/**
 * Media Preload API
 */
interface IMediaPreload {

    /** add the preload status listener */
    fun addPreloadCacheListener(listener: PreloadCacheListener?)

    /** remove the preload status listener */
    fun removePreloadCacheListener(listener: PreloadCacheListener?)

    /** start all preload task */
    fun start(requests: List<VideoCacheRequest>?)

    /** stop all preload task */
    fun stop()

    fun release()

    /** get the dir's all files */
    fun getAllCacheFilePaths(cacheDir: String?): Array<String>?

    /** delete the cache dir task: the url is the key of preload task */
    fun deleteCacheFile(url: String?, cacheDir: String?): Int?

    /** get the file real download size: not the file size */
    fun getCacheFileRealSize(url: String?, cacheDir: String?): Long?

    /** get the preload url's file absolute path */
    fun getCacheFilePath(url: String?, cacheDir: String?): String?
}
