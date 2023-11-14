package com.xingin.openredpreload.api

import com.xingin.openredpreload.impl.model.VideoCacheRequest

/** Preload Task Callback */
interface PreloadCacheListener {

    /**
     * @param cacheFilePath file absolute path
     * @param cacheFileSize file size, unit: b
     * @param request origin preload request
     */
    fun onPreloadSuccess(cacheFilePath: String, cacheFileSize: Long, request: VideoCacheRequest) {}

    /**
     * @param errorMsg preload error msg
     * @param request origin preload request
     */
    fun onPreloadError(errorMsg: Long, request: VideoCacheRequest) {}

    /**
     * @param request origin preload request
     */
    fun onPreloadStart(request: VideoCacheRequest) {}
}