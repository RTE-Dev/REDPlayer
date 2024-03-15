package com.xingin.openredpreload.impl.model

import android.os.Bundle

/**
 * preload request model
 */
data class VideoCacheRequest(
    /** preload task name */
    val name: String = "",
    /** preload resource url */
    val videoUrl: String = "",
    /** preload resource json url, high than videoUrl */
    val videoJson: String = "",
    /** preload cache dir */
    val cacheDir: String = "",
    /** preload data size */
    val cacheSize: Long = 0,
    /** preload dir max size: when beyond, start LRU */
    val cacheDirMaxSize: Long = 0,
    /** preload dir max file count: when beyond,start LRU */
    val cacheDirMaxEntries: Long = 0,
    val businessLine: String = "",
) {
    val finalUrl = videoJson.ifBlank { videoUrl }

    fun getPreloadParams(): Bundle {
        return Bundle().apply {
            if (videoJson.isNotBlank()) {
                this.putInt("is_json", 1)
            }
            // network: same the http request param
            this.putInt("use_https", 1)
            this.putString("referer", "")
            this.putString("user_agent", "")
            this.putString("header", "")
            this.putLong("capacity", cacheDirMaxSize)
            this.putLong("cache_max_entries", cacheDirMaxEntries)
        }
    }
}