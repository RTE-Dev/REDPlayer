package com.xingin.openredplayer.player.progress;

import androidx.annotation.ColorInt;
import androidx.annotation.IntRange;

import java.text.NumberFormat;

public class XhsProgressBuilder {
    float min;
    float max;
    float progress;
    boolean floatType;
    int trackSize;
    int secondTrackSize;
    int thumbRadius;
    int thumbRadiusOnDragging;
    int trackColor;
    int secondTrackColor;
    int thumbColor;
    int sectionCount;
    boolean showSectionMark;
    boolean autoAdjustSectionMark;
    boolean showSectionText;
    int sectionTextSize;
    int sectionTextColor;
    @XhsProgressBar.TextPosition
    int sectionTextPosition;
    int sectionTextInterval;
    boolean showThumbText;
    int thumbTextSize;
    int thumbTextColor;
    boolean showProgressInFloat;
    long animDuration;
    boolean touchToSeek;
    boolean seekBySection;
    int signColor;
    int signTextSize;
    int signTextColor;
    boolean showSign;
    String[] bottomSidesLabels;
    float thumbBgAlpha; //  alpha of thumb shadow
    float thumbRatio; // ratio of thumb shadow
    boolean showThumbShadow;
    XhsProgressBar mSignSeekBar;
    String unit;
    int signArrowHeight;
    int signArrowWidth;
    int signRound;
    int signHeight; //sign Height
    int signWidth; //sign width
    int signBorderSize; // border size
    boolean showSignBorder; // show sign border
    boolean signArrowAutofloat;
    int signBorderColor;// color of border color
    NumberFormat format;
    boolean reverse;

    XhsProgressBuilder(XhsProgressBar xhsSeekBar) {
        mSignSeekBar = xhsSeekBar;
    }

    public void build() {
        mSignSeekBar.config(this);
    }

    public XhsProgressBuilder min(float min) {
        this.min = min;
        this.progress = min;
        return this;
    }

    public XhsProgressBuilder max(float max) {
        this.max = max;
        return this;
    }

    public XhsProgressBuilder progress(float progress) {
        this.progress = progress;
        return this;
    }

    public XhsProgressBuilder floatType() {
        this.floatType = true;
        return this;
    }

    public XhsProgressBuilder trackSize(int dp) {
        this.trackSize = XhsProgressUtils.dp2px(dp);
        return this;
    }

    public XhsProgressBuilder secondTrackSize(int dp) {
        this.secondTrackSize = XhsProgressUtils.dp2px(dp);
        return this;
    }

    public XhsProgressBuilder thumbRadius(int dp) {
        this.thumbRadius = XhsProgressUtils.dp2px(dp);
        return this;
    }

    public XhsProgressBuilder thumbRadiusOnDragging(int dp) {
        this.thumbRadiusOnDragging = XhsProgressUtils.dp2px(dp);
        return this;
    }

    public XhsProgressBuilder trackColor(@ColorInt int color) {
        this.trackColor = color;
        this.sectionTextColor = color;
        return this;
    }

    public XhsProgressBuilder secondTrackColor(@ColorInt int color) {
        this.secondTrackColor = color;
        this.thumbColor = color;
        this.thumbTextColor = color;
        this.signColor = color;
        return this;
    }

    public XhsProgressBuilder thumbColor(@ColorInt int color) {
        this.thumbColor = color;
        return this;
    }

    public XhsProgressBuilder sectionCount(@IntRange(from = 1) int count) {
        this.sectionCount = count;
        return this;
    }

    public XhsProgressBuilder showSectionMark() {
        this.showSectionMark = true;
        return this;
    }

    public XhsProgressBuilder autoAdjustSectionMark() {
        this.autoAdjustSectionMark = true;
        return this;
    }

    public XhsProgressBuilder showSectionText() {
        this.showSectionText = true;
        return this;
    }

    public XhsProgressBuilder sectionTextSize(int sp) {
        this.sectionTextSize = XhsProgressUtils.sp2px(sp);
        return this;
    }

    public XhsProgressBuilder sectionTextColor(@ColorInt int color) {
        this.sectionTextColor = color;
        return this;
    }

    public XhsProgressBuilder sectionTextPosition(@XhsProgressBar.TextPosition int position) {
        this.sectionTextPosition = position;
        return this;
    }

    public XhsProgressBuilder sectionTextInterval(@IntRange(from = 1) int interval) {
        this.sectionTextInterval = interval;
        return this;
    }

    public XhsProgressBuilder showThumbText() {
        this.showThumbText = true;
        return this;
    }

    public XhsProgressBuilder thumbTextSize(int sp) {
        this.thumbTextSize = XhsProgressUtils.sp2px(sp);
        return this;
    }

    public XhsProgressBuilder thumbTextColor(@ColorInt int color) {
        thumbTextColor = color;
        return this;
    }

    public XhsProgressBuilder showProgressInFloat() {
        this.showProgressInFloat = true;
        return this;
    }

    public XhsProgressBuilder animDuration(long duration) {
        animDuration = duration;
        return this;
    }

    public XhsProgressBuilder touchToSeek() {
        this.touchToSeek = true;
        return this;
    }

    public XhsProgressBuilder seekBySection() {
        this.seekBySection = true;
        return this;
    }


    public XhsProgressBuilder bottomSidesLabels(String[] bottomSidesLabels) {
        this.bottomSidesLabels = bottomSidesLabels;
        return this;
    }

    public XhsProgressBuilder thumbBgAlpha(float thumbBgAlpha) {
        this.thumbBgAlpha = thumbBgAlpha;
        return this;
    }

    public XhsProgressBuilder thumbRatio(float thumbRatio) {
        this.thumbRatio = thumbRatio;
        return this;
    }

    public XhsProgressBuilder showThumbShadow(boolean showThumbShadow) {
        this.showThumbShadow = showThumbShadow;
        return this;
    }

    public XhsProgressBuilder signColor(@ColorInt int color) {
        this.signColor = color;
        return this;
    }

    public XhsProgressBuilder signTextSize(int sp) {
        this.signTextSize = XhsProgressUtils.sp2px(sp);
        return this;
    }

    public XhsProgressBuilder signTextColor(@ColorInt int color) {
        this.signTextColor = color;
        return this;
    }

    public XhsProgressBuilder showSign() {
        this.showSign = true;
        return this;
    }

    public XhsProgressBuilder signArrowHeight(int signArrowHeight) {
        this.signArrowHeight = signArrowHeight;
        return this;
    }

    public XhsProgressBuilder signArrowWidth(int signArrowWidth) {
        this.signArrowWidth = signArrowWidth;
        return this;
    }

    public XhsProgressBuilder signRound(int signRound) {
        this.signRound = signRound;
        return this;
    }

    public XhsProgressBuilder signHeight(int signHeight) {
        this.signHeight = signHeight;
        return this;
    }

    public XhsProgressBuilder signWidth(int signWidth) {
        this.signWidth = signWidth;
        return this;
    }

    public XhsProgressBuilder signBorderSize(int signBorderSize) {
        this.signBorderSize = signBorderSize;
        return this;
    }

    public XhsProgressBuilder showSignBorder(boolean showSignBorder) {
        this.showSignBorder = showSignBorder;
        return this;
    }

    public XhsProgressBuilder signBorderColor(int signBorderColor) {
        this.signBorderColor = signBorderColor;
        return this;
    }

    public XhsProgressBuilder signArrowAutofloat(boolean signArrowAutofloat) {
        this.signArrowAutofloat = signArrowAutofloat;
        return this;
    }

    public float getMin() {
        return min;
    }

    public float getMax() {
        return max;
    }

    public float getProgress() {
        return progress;
    }

    public boolean isFloatType() {
        return floatType;
    }

    public int getTrackSize() {
        return trackSize;
    }

    public int getSecondTrackSize() {
        return secondTrackSize;
    }

    public int getThumbRadius() {
        return thumbRadius;
    }

    public int getThumbRadiusOnDragging() {
        return thumbRadiusOnDragging;
    }

    public int getTrackColor() {
        return trackColor;
    }

    public int getSecondTrackColor() {
        return secondTrackColor;
    }

    public int getThumbColor() {
        return thumbColor;
    }

    public int getSectionCount() {
        return sectionCount;
    }

    public boolean isShowSectionMark() {
        return showSectionMark;
    }

    public boolean isAutoAdjustSectionMark() {
        return autoAdjustSectionMark;
    }

    public boolean isShowSectionText() {
        return showSectionText;
    }

    public int getSectionTextSize() {
        return sectionTextSize;
    }

    public int getSectionTextColor() {
        return sectionTextColor;
    }

    public int getSectionTextPosition() {
        return sectionTextPosition;
    }

    public int getSectionTextInterval() {
        return sectionTextInterval;
    }

    public boolean isShowThumbText() {
        return showThumbText;
    }

    public int getThumbTextSize() {
        return thumbTextSize;
    }

    public int getThumbTextColor() {
        return thumbTextColor;
    }

    public boolean isShowProgressInFloat() {
        return showProgressInFloat;
    }

    public long getAnimDuration() {
        return animDuration;
    }

    public boolean isTouchToSeek() {
        return touchToSeek;
    }

    public boolean isSeekBySection() {
        return seekBySection;
    }

    public String[] getBottomSidesLabels() {
        return bottomSidesLabels;
    }

    public float getThumbBgAlpha() {
        return thumbBgAlpha;
    }

    public float getThumbRatio() {
        return thumbRatio;
    }

    public boolean isShowThumbShadow() {
        return showThumbShadow;
    }

    public XhsProgressBuilder setUnit(String unit) {
        this.unit = unit;
        return this;
    }

    public int getSignColor() {
        return signColor;
    }

    public int getSignTextSize() {
        return signTextSize;
    }

    public int getSignTextColor() {
        return signTextColor;
    }


    public boolean isshowSign() {
        return showSign;
    }

    public String getUnit() {
        return unit;
    }

    public int getSignArrowHeight() {
        return signArrowHeight;
    }

    public int getSignArrowWidth() {
        return signArrowWidth;
    }

    public int getSignRound() {
        return signRound;
    }

    public int getSignHeight() {
        return signHeight;
    }

    public int getSignWidth() {
        return signWidth;
    }

    public int getSignBorderSize() {
        return signBorderSize;
    }

    public boolean isShowSignBorder() {
        return showSignBorder;
    }

    public int getSignBorderColor() {
        return signBorderColor;
    }

    public boolean isSignArrowAutofloat() {
        return signArrowAutofloat;
    }

    public XhsProgressBuilder format(NumberFormat format) {
        this.format = format;
        return this;
    }

    public NumberFormat getFormat() {
        return format;
    }

    public boolean isReverse() {
        return reverse;
    }

    public XhsProgressBuilder reverse() {
        this.reverse = true;
        return this;
    }
}
