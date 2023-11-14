package com.xingin.openredplayer.player.section

import android.graphics.Rect
import android.view.View
import androidx.recyclerview.widget.RecyclerView

class XhsSectionSpaceItemDecoration(private val verticalSpaceHeight: Int) :
    RecyclerView.ItemDecoration() {

    override fun getItemOffsets(
        outRect: Rect,
        view: View,
        parent: RecyclerView,
        state: RecyclerView.State
    ) {
        outRect.bottom = verticalSpaceHeight
    }
}