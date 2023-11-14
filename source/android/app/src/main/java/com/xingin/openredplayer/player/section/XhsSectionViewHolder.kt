package com.xingin.openredplayer.player.section

import android.view.View
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.wang.avi.AVLoadingIndicatorView
import com.xingin.openredplayer.R

class XhsSectionViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    var textView: TextView
    var loadingIndicatorView: AVLoadingIndicatorView

    init {
        textView = itemView.findViewById(R.id.tv)
        loadingIndicatorView = itemView.findViewById(R.id.loading_view)
    }
}