package com.xingin.openredplayer.feed

import android.annotation.SuppressLint
import android.app.Activity
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.core.content.contentValuesOf
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.xingin.openredplayer.R
import com.xingin.openredplayer.feed.model.XhsFeedModel

class XhsFeedAdapter(val context: Activity, var items: MutableList<XhsFeedModel>) :
    RecyclerView.Adapter<XhsFeedViewHolder>() {
    private var mOnItemClickListener: OnItemClickListener? = null
    private var mOnItemLongClickListener: OnItemLongClickListener? = null
    private var itemCount = 0

    init {
        itemCount = items.size
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): XhsFeedViewHolder {
        val item =
            LayoutInflater.from(parent.context).inflate(R.layout.item_feed_layout, parent, false)
        return XhsFeedViewHolder(item)
    }

    @SuppressLint("UseCompatLoadingForDrawables")
    override fun onBindViewHolder(holder: XhsFeedViewHolder, position: Int) {
        val model = items[position]
        // holder.imageView.setImageResource(model.coverUrl)
        Glide.with(context).load("").placeholder(model.coverUrl).into(holder.imageView)
        holder.itemView.setOnClickListener {
            mOnItemClickListener?.onClick(holder, position, model)
        }
        holder.itemView.setOnLongClickListener {
            mOnItemLongClickListener?.onLongClick(holder, position, model)
            return@setOnLongClickListener true
        }
    }

    override fun getItemCount(): Int {
        return items.size
    }

    fun setOnItemClickListener(onItemClickListener: OnItemClickListener?) {
        mOnItemClickListener = onItemClickListener
    }

    fun setOnItemLongClickListener(onItemLongClickListener: OnItemLongClickListener?) {
        mOnItemLongClickListener = onItemLongClickListener
    }

    fun loadMoreData(feeds: List<XhsFeedModel>) {
        val oldItemCount: Int = itemCount
        items.addAll(feeds)
        itemCount = items.size
        notifyItemRangeInserted(oldItemCount, feeds.size)
    }

    interface OnItemClickListener {
        fun onClick(holder: RecyclerView.ViewHolder?, position: Int, model: XhsFeedModel?)
    }

    interface OnItemLongClickListener {
        fun onLongClick(holder: RecyclerView.ViewHolder?, position: Int, model: XhsFeedModel?)
    }
}