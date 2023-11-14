package com.xingin.openredplayer.feed

import android.view.View
import android.widget.ImageView
import androidx.recyclerview.widget.RecyclerView
import com.xingin.openredplayer.R
import com.xingin.openredplayer.feed.view.XhsFeedCoverPlayerView

class XhsFeedViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    var imageView: ImageView
    var videoPlayerView: XhsFeedCoverPlayerView

    init {
        imageView = itemView.findViewById(R.id.iv)
        videoPlayerView = itemView.findViewById(R.id.feed_video_view)
    }
}