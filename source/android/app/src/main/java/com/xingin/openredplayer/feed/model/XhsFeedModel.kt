package com.xingin.openredplayer.feed.model

import com.kelin.photoselector.model.Photo
import com.xingin.openredplayer.R
import com.xingin.openredplayer.utils.Utils

/** define the feed model */
data class XhsFeedModel(
    val coverUrl: Int,
    val isLocal: Boolean,
    val videoUrl: String? = null,
    val videoJsonUrl: String? = null,
    val isShowLoading: Boolean? = false,
    val canPlay: Boolean? = true,
) {
    val finalUrl = if (videoJsonUrl.isNullOrEmpty()) videoUrl else videoJsonUrl
}

/** init the rv data model */
fun getHomeFeedData(): MutableList<XhsFeedModel> {
    return ArrayList<XhsFeedModel>().apply {
        // JSON
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_1,
                isLocal = false,
                jsonVideoUrl = JSON_DATA_SOURCE,
                isShowLoading = false,
                canPlay = true,
            )
        )
        // MP4
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_2,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = true,
            )
        )
        // MP4-HDR
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_3,
                isLocal = false,
                videoUrl = VIDEO_TWO,
                isShowLoading = false,
                canPlay = true,
            )
        )
        // HLS
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_4,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = true,
            )
        )
        // HLS-Live-4k
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_5,
                isLocal = false,
                videoUrl = VIDEO_TWO,
                isShowLoading = false,
                canPlay = false,
            )
        )
        // RTMP
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_6,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = false,
            )
        )
        // m3u8
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_7,
                isLocal = false,
                videoUrl = VIDEO_TWO,
                isShowLoading = false,
                canPlay = true,
            )
        )
        // MOV
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_8,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = true,
            )
        )
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_9,
                isLocal = false,
                videoUrl = VIDEO_TWO,
                isShowLoading = false,
                canPlay = true,
            )
        )
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_1,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = true,
            )
        )
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_2,
                isLocal = false,
                videoUrl = VIDEO_TWO,
                isShowLoading = false,
                canPlay = true,
            )
        )
        this.add(
            createModel(
                coverUrl = R.mipmap.icon_video_3,
                isLocal = false,
                videoUrl = VIDEO_ONE,
                isShowLoading = false,
                canPlay = true,
            )
        )
    }
}

private fun createModel(
    coverUrl: Int,
    isLocal: Boolean,
    videoUrl: String? = null,
    jsonVideoUrl: String? = null,
    isShowLoading: Boolean? = false,
    canPlay: Boolean? = false,
): XhsFeedModel {
    return XhsFeedModel(coverUrl, isLocal, videoUrl, jsonVideoUrl, isShowLoading, canPlay)
}

/** the video source */
val VIDEO_ONE =
    Utils.decodeBase64("aHR0cHM6Ly9zbnMtdmlkZW8tYWwueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzQwNS8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4YzhhZDk2MmZiXzQwNS5tcDQ=")
val VIDEO_TWO =
    Utils.decodeBase64("aHR0cDovL3Nucy12aWRlby1hbC54aHNjZG4uY29tL3NwZWN0cnVtLzAxZTI5NjRlNDE2YmUwNzcwMTgzNzAwMzgxMWI0ZTM5YzlfNTEzLm1wNA==")

/** the json data source */
val JSON_DATA_SOURCE = Utils.decodeBase64(
    "eyJzdHJlYW0iOnsiaDI2NCI6W3sid2lkdGgiOjcyMCwiaGVpZ2h0IjoxMjgwLCJhdmdfYml0cmF0ZSI6MTAzNTE1NCwibWFzdGVyX3VybCI6Imh0dHBzOi8vc25zLXZpZGVvLWFsLnhoc2Nkbi5jb20vc3RyZWFtLzExMC80MDUvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM4YWQ5NjJmYl80MDUubXA0IiwiYmFja3VwX3VybHMiOlsiaHR0cHM6Ly9zbnMtdmlkZW8tYWwueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzQwNS8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4YzhhZDk2MmZiXzQwNS5tcDQiXX1dLCJoMjY1IjpbeyJ3aWR0aCI6NzIwLCJoZWlnaHQiOjEyODAsImF2Z19iaXRyYXRlIjo2ODcwMjYsIm1hc3Rlcl91cmwiOiJodHRwczovL3Nucy12aWRlby1hbC54aHNjZG4uY29tL3N0cmVhbS8xMTAvNDA1LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOGFkOTYyZmJfNDA1Lm1wNCIsImJhY2t1cF91cmxzIjpbImh0dHBzOi8vc25zLXZpZGVvLWFsLnhoc2Nkbi5jb20vc3RyZWFtLzExMC80MDUvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM4YWQ5NjJmYl80MDUubXA0Il19LHsid2lkdGgiOjEwODAsImhlaWdodCI6MTkyMCwiYXZnX2JpdHJhdGUiOjkzOTU1NCwibWFzdGVyX3VybCI6Imh0dHBzOi8vc25zLXZpZGVvLWFsLnhoc2Nkbi5jb20vc3RyZWFtLzExMC80MDUvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM4YWQ5NjJmYl80MDUubXA0IiwiYmFja3VwX3VybHMiOlsiaHR0cHM6Ly9zbnMtdmlkZW8tYWwueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzQwNS8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4YzhhZDk2MmZiXzQwNS5tcDQiXX1dfX0="
)

fun covertToVideoUrls(photos: List<Photo>?): List<String> {
    val urls = arrayListOf<String>()
    photos?.forEach { urls.add(it.uri) }
    return urls
}

