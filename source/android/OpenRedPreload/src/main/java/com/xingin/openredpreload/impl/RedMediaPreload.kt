package com.xingin.openredpreload.impl

import android.os.Bundle
import android.util.Log
import com.xingin.openredpreload.OpenRedPreload
import com.xingin.openredpreload.api.IMediaPreload
import com.xingin.openredpreload.api.PreloadCacheListener
import com.xingin.openredpreload.impl.model.VideoCacheRequest
import java.lang.ref.WeakReference
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.CopyOnWriteArrayList

/**
 * Preload Controller: submit tasks, stop, release and listen preload state
 */
class RedMediaPreload : IMediaPreload {
    private val TAG = "RedMediaPreload"

    // inner preload core
    private val internalPreload: OpenRedPreload = OpenRedPreload()

    private var preloadCacheListeners = CopyOnWriteArrayList<PreloadCacheListener>()

    private val downloadEventListener =
        OpenRedPreload.DownloadEventListener { event: Int, bundle: Bundle?, realCacheUrl: String?, userData: Any? ->
            when (event) {
                // preload task success
                OpenRedPreload.PRELOAD_EVENT_IO_TRAFFIC -> {
                    if (bundle == null) {
                        return@DownloadEventListener
                    }
                    if (userData !is VideoCacheRequest) {
                        return@DownloadEventListener
                    }
                    val cacheSize = bundle.getInt("cache_size").toLong()
                    val localPath = bundle.getString("cache_file_path", "")
                    preloadCacheListeners.forEach { listener ->
                        listener.onPreloadSuccess(localPath, cacheSize, userData)
                    }
                    Log.d(
                        TAG, "[DownloadEventListener]: PRELOAD_EVENT_IO_TRAFFIC:$userData"
                    )
                }

                // Preload Speed
                OpenRedPreload.PRELOAD_EVENT_SPEED -> {
                    if (bundle == null) {
                        return@DownloadEventListener
                    }
                    if (userData !is VideoCacheRequest) {
                        return@DownloadEventListener
                    }
                    val speed = bundle.getInt("speed")
                    Log.d(
                        TAG,
                        "[DownloadEventListener]: speed:${speed}, $userData"
                    )
                }

                // TCP Connection
                OpenRedPreload.PRELOAD_EVENT_DID_TCP_OPEN -> {
                    if (bundle == null) {
                        return@DownloadEventListener
                    }
                    if (userData !is VideoCacheRequest) {
                        return@DownloadEventListener
                    }
                    val dstIp = bundle.getString("dst_ip") ?: ""
                    val source = bundle.getInt("source")
                    Log.d(
                        TAG,
                        "[DownloadEventListener]：PRELOAD_EVENT_DID_TCP_OPEN, dstIp:$dstIp, source:$source, userData:$userData"
                    )
                }

                // HTTP Connection Success
                OpenRedPreload.PRELOAD_EVENT_DID_HTTP_OPEN -> {
                    if (bundle == null) {
                        return@DownloadEventListener
                    }
                    if (userData !is VideoCacheRequest) {
                        return@DownloadEventListener
                    }
                    val httpRtt = bundle.getInt("http_rtt").toLong()
                    val source = bundle.getInt("source")
                    Log.d(
                        TAG, "[DownloadEventListener]：" +
                                "PRELOAD_EVENT_DID_HTTP_OPEN, httpRtt:$httpRtt source:$source userData:$userData"
                    )
                }

                // 预载出错
                OpenRedPreload.PRELOAD_EVENT_ERROR -> {
                    if (bundle == null) {
                        return@DownloadEventListener
                    }
                    if (userData !is VideoCacheRequest) {
                        return@DownloadEventListener
                    }
                    val error = bundle.getInt("error").toLong()
                    val source = bundle.getInt("source")
                    preloadCacheListeners.forEach { listener ->
                        listener.onPreloadError(error, userData)
                    }
                    Log.d(
                        TAG, "[DownloadEventListener]：" +
                                "PRELOAD_EVENT_ERROR, error:$error, source:$source, userData:$userData"
                    )
                }
            }
        }

    override fun addPreloadCacheListener(listener: PreloadCacheListener?) {
        listener?.let { preloadCacheListeners.add(it) }
    }

    override fun removePreloadCacheListener(listener: PreloadCacheListener?) {
        listener?.let { preloadCacheListeners.remove(it) }
    }

    override fun start(requests: List<VideoCacheRequest>?) {
        if (requests?.isEmpty() == true) {
            Log.d(TAG, "has no preload task, return.")
            return
        }
        submitNewPreloadRequest(requests)
    }

    override fun stop() {
        internalPreload.stop()
        Log.d(TAG, "stop(), stop all preload task.")
    }

    override fun release() {
        internalPreload.release()
        preloadCacheListeners.clear()
        Log.d(TAG, "release(), release all task.")
    }

    override fun getAllCacheFilePaths(cacheDir: String?): Array<String>? {
        if (cacheDir.isNullOrEmpty()) {
            return null
        }
        return internalPreload.getAllCachedFile(cacheDir)?.also {
            Log.d(
                TAG,
                "getAllCacheFilePaths(), cacheDir: $cacheDir, result: ${it.size}"
            )
        }
    }

    override fun deleteCacheFile(url: String?, cacheDir: String?): Int? {
        if (url.isNullOrEmpty() || cacheDir.isNullOrEmpty()) {
            return -1
        }
        return internalPreload.deleteCache(cacheDir, url, true).also {
            Log.d(
                TAG,
                "deleteCacheFile(), url: $url, cacheDir: $cacheDir, result: $it"
            )
        }
    }

    override fun getCacheFileRealSize(url: String?, cacheDir: String?): Long {
        if (url.isNullOrEmpty() || cacheDir.isNullOrEmpty()) {
            return -1L
        }
        return internalPreload.getCachedSize(cacheDir, url, true).also {
            Log.d(
                TAG,
                "getCacheFileRealSize(), url: $url, cacheDir: $cacheDir, fileSize: $it"
            )
        }
    }

    override fun getCacheFilePath(url: String?, cacheDir: String?): String? {
        if (url.isNullOrEmpty() || cacheDir.isNullOrEmpty()) {
            return null
        }
        return internalPreload.getCacheFilePath(cacheDir, url)?.also {
            Log.d(
                TAG,
                "getCacheFilePath(), cacheDir: $cacheDir, url: ${url}, localPath: $it"
            )
        }
    }

    /** submit new preload request */
    private fun submitNewPreloadRequest(requests: List<VideoCacheRequest>?) {
        requests?.forEach { request ->
            internalPreload.open(
                request.finalUrl,
                request.cacheDir,
                request.cacheSize,
                WeakReference(downloadEventListener),
                request.getPreloadParams(),
                request
            )
        }
    }
}