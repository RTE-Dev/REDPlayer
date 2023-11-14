package com.xingin.openredplayercore.core.impl;

import com.xingin.openredplayercore.core.api.IMediaPlayer;

public abstract class AbstractMediaPlayer implements IMediaPlayer {
    private IMediaPlayer.OnPreparedListener mOnPreparedListener;
    private IMediaPlayer.OnCompletionListener mOnCompletionListener;
    private IMediaPlayer.OnBufferingUpdateListener mOnBufferingUpdateListener;
    private IMediaPlayer.OnSeekCompleteListener mOnSeekCompleteListener;
    private IMediaPlayer.OnVideoSizeChangedListener mOnVideoSizeChangedListener;
    private IMediaPlayer.OnErrorListener mOnErrorListener;
    private IMediaPlayer.OnInfoListener mOnInfoListener;

    public AbstractMediaPlayer() {
    }

    @Override
    public final void setOnPreparedListener(IMediaPlayer.OnPreparedListener listener) {
        this.mOnPreparedListener = listener;
    }

    @Override
    public final void setOnCompletionListener(IMediaPlayer.OnCompletionListener listener) {
        this.mOnCompletionListener = listener;
    }

    @Override
    public final void setOnBufferingUpdateListener(IMediaPlayer.OnBufferingUpdateListener listener) {
        this.mOnBufferingUpdateListener = listener;
    }

    @Override
    public final void setOnSeekCompleteListener(IMediaPlayer.OnSeekCompleteListener listener) {
        this.mOnSeekCompleteListener = listener;
    }

    @Override
    public final void setOnVideoSizeChangedListener(IMediaPlayer.OnVideoSizeChangedListener listener) {
        this.mOnVideoSizeChangedListener = listener;
    }

    @Override
    public final void setOnErrorListener(IMediaPlayer.OnErrorListener listener) {
        this.mOnErrorListener = listener;
    }

    @Override
    public final void setOnInfoListener(IMediaPlayer.OnInfoListener listener) {
        this.mOnInfoListener = listener;
    }

    public void resetListeners() {
        this.mOnPreparedListener = null;
        this.mOnBufferingUpdateListener = null;
        this.mOnCompletionListener = null;
        this.mOnSeekCompleteListener = null;
        this.mOnVideoSizeChangedListener = null;
        this.mOnErrorListener = null;
        this.mOnInfoListener = null;
    }

    protected final void notifyOnPrepared(RedPlayerEvent event) {
        if (this.mOnPreparedListener != null) {
            this.mOnPreparedListener.onPrepared(this, event);
        }
    }

    protected final void notifyOnCompletion() {
        if (this.mOnCompletionListener != null) {
            this.mOnCompletionListener.onCompletion(this);
        }
    }

    protected void notifyOnBufferingUpdate(int percent) {
        if (this.mOnBufferingUpdateListener != null) {
            this.mOnBufferingUpdateListener.onBufferingUpdate(this, percent);
        }
    }

    protected void notifyOnSeekComplete() {
        if (this.mOnSeekCompleteListener != null) {
            this.mOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    protected void notifyOnVideoSizeChanged(int width, int height, int sarNum, int sarDen) {
        if (this.mOnVideoSizeChangedListener != null) {
            this.mOnVideoSizeChangedListener.onVideoSizeChanged(this, width, height, sarNum, sarDen);
        }
    }

    protected boolean notifyOnError(int what, int extra) {
        return this.mOnErrorListener != null && this.mOnErrorListener.onError(this, what, extra);
    }

    protected boolean notifyOnInfo(int what, int extra, RedPlayerEvent obj) {
        return mOnInfoListener != null && mOnInfoListener.onInfo(this, what, extra, obj);
    }
}
