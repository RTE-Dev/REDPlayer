package com.xingin.openredplayercore.core.renderview;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.xingin.openredplayercore.core.api.IMediaPlayer;
import com.xingin.openredplayercore.core.renderview.helper.MeasureHelper;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * SurfaceView渲染
 */
public class SurfaceRenderView extends SurfaceView implements IRenderView {
    private MeasureHelper mMeasureHelper;

    public SurfaceRenderView(Context context) {
        super(context);
        initView(context);
    }

    public SurfaceRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView(context);
    }

    public SurfaceRenderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView(context);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public SurfaceRenderView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initView(context);
    }

    private void initView(Context context) {
        mMeasureHelper = new MeasureHelper(this);
        mSurfaceCallback = new SurfaceCallback(this);
        getHolder().addCallback(mSurfaceCallback);
        getHolder().setType(SurfaceHolder.SURFACE_TYPE_NORMAL);
    }

    @Override
    public View getView() {
        return this;
    }

    @Override
    public boolean shouldWaitForResize() {
        return true;
    }

    @Override
    public void setVideoSize(int videoWidth, int videoHeight) {
        if (videoWidth > 0 && videoHeight > 0) {
            mMeasureHelper.setVideoSize(videoWidth, videoHeight);
            getHolder().setFixedSize(videoWidth, videoHeight);
            requestLayout();
        }
    }

    @Override
    public void setVideoSampleAspectRatio(int videoSarNum, int videoSarDen) {
        if (videoSarNum > 0 && videoSarDen > 0) {
            mMeasureHelper.setVideoSampleAspectRatio(videoSarNum, videoSarDen);
            requestLayout();
        }
    }

    @Override
    public void setVideoRotation(int degree) {
        Log.e("", "SurfaceView doesn't support rotation (" + degree + ")!\n");
    }

    @Override
    public void setAspectRatio(int aspectRatio) {
        mMeasureHelper.setAspectRatio(aspectRatio);
        requestLayout();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        mMeasureHelper.doMeasure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(mMeasureHelper.getMeasuredWidth(), mMeasureHelper.getMeasuredHeight());
    }

    private static final class InternalSurfaceHolder implements IRenderView.ISurfaceHolder {
        private SurfaceRenderView mSurfaceView;
        private SurfaceHolder mSurfaceHolder;

        public InternalSurfaceHolder(@NonNull SurfaceRenderView surfaceView,
                                     @Nullable SurfaceHolder surfaceHolder) {
            mSurfaceView = surfaceView;
            mSurfaceHolder = surfaceHolder;
        }

        public void bindToMediaPlayer(IMediaPlayer mp) {
            if (mp != null) {
                mp.setDisplay(mSurfaceHolder);
            }
        }

        @NonNull
        @Override
        public IRenderView getRenderView() {
            return mSurfaceView;
        }

        @Nullable
        @Override
        public SurfaceHolder getSurfaceHolder() {
            return mSurfaceHolder;
        }

        @Nullable
        @Override
        public SurfaceTexture getSurfaceTexture() {
            return null;
        }

        @Nullable
        @Override
        public Surface openSurface() {
            if (mSurfaceHolder == null)
                return null;
            return mSurfaceHolder.getSurface();
        }
    }

    @Override
    public void addRenderCallback(IRenderCallback callback) {
        mSurfaceCallback.addRenderCallback(callback);
    }

    @Override
    public void removeRenderCallback(IRenderCallback callback) {
        mSurfaceCallback.removeRenderCallback(callback);
    }

    private SurfaceCallback mSurfaceCallback;

    private static final class SurfaceCallback implements SurfaceHolder.Callback {
        private SurfaceHolder mSurfaceHolder;
        private boolean mIsFormatChanged;
        private int mFormat;
        private int mWidth;
        private int mHeight;

        private WeakReference<SurfaceRenderView> mWeakSurfaceView;
        private Map<IRenderCallback, Object> mRenderCallbackMap = new ConcurrentHashMap<IRenderCallback, Object>();

        public SurfaceCallback(@NonNull SurfaceRenderView surfaceView) {
            mWeakSurfaceView = new WeakReference<SurfaceRenderView>(surfaceView);
        }

        public void addRenderCallback(@NonNull IRenderCallback callback) {
            mRenderCallbackMap.put(callback, callback);
            ISurfaceHolder surfaceHolder = null;
            if (mSurfaceHolder != null) {
                if (surfaceHolder == null)
                    surfaceHolder = new InternalSurfaceHolder(mWeakSurfaceView.get(), mSurfaceHolder);
                callback.onSurfaceCreated(surfaceHolder, mWidth, mHeight);
            }

            if (mIsFormatChanged) {
                if (surfaceHolder == null)
                    surfaceHolder = new InternalSurfaceHolder(mWeakSurfaceView.get(), mSurfaceHolder);
                callback.onSurfaceChanged(surfaceHolder, mFormat, mWidth, mHeight);
            }
        }

        public void removeRenderCallback(@NonNull IRenderCallback callback) {
            mRenderCallbackMap.remove(callback);
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mSurfaceHolder = holder;
            mIsFormatChanged = false;
            mFormat = 0;
            mWidth = 0;
            mHeight = 0;

            ISurfaceHolder surfaceHolder = new InternalSurfaceHolder(mWeakSurfaceView.get(), mSurfaceHolder);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceCreated(surfaceHolder, 0, 0);
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mSurfaceHolder = null;
            mIsFormatChanged = false;
            mFormat = 0;
            mWidth = 0;
            mHeight = 0;
            ISurfaceHolder surfaceHolder = new InternalSurfaceHolder(mWeakSurfaceView.get(), mSurfaceHolder);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceDestroyed(surfaceHolder);
            }
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            mSurfaceHolder = holder;
            mIsFormatChanged = true;
            mFormat = format;
            mWidth = width;
            mHeight = height;
            ISurfaceHolder surfaceHolder = new InternalSurfaceHolder(mWeakSurfaceView.get(), mSurfaceHolder);
            for (IRenderCallback renderCallback : mRenderCallbackMap.keySet()) {
                renderCallback.onSurfaceChanged(surfaceHolder, format, width, height);
            }
        }
    }

    @Override
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);
        event.setClassName(SurfaceRenderView.class.getName());
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(SurfaceRenderView.class.getName());
    }
}
