#pragma once

#if defined(__ANDROID__)

#include <jni.h>

jobject JniJavaNioByteBufferAllocate(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateCatchAll(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateGlobalRef(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateGlobalRefCatchAll(JNIEnv *env,
                                                      jint capacity);
jobject JniJavaNioByteBufferAllocateDirect(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateDirectCatchAll(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateDirectGlobalRef(JNIEnv *env, jint capacity);
jobject JniJavaNioByteBufferAllocateDirectGlobalRefCatchAll(JNIEnv *env,
                                                            jint capacity);
jobject JniJavaNioByteBufferLimit(JNIEnv *env, jobject thiz, jint new_limit);
jobject JniJavaNioByteBufferLimitCatchAll(JNIEnv *env, jobject thiz,
                                          jint new_limit);
jobject JniJavaNioByteBufferLimitGlobalRef(JNIEnv *env, jobject thiz,
                                           jint new_limit);
jobject JniJavaNioByteBufferLimitGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                   jint new_limit);
int JniLoadClassJavaNioByteBuffer(JNIEnv *env);

#endif
