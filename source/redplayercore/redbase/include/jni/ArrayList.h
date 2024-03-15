#pragma once

#if defined(__ANDROID__)

#include <jni.h>

jobject JniJavaUtilArrayList(JNIEnv *env);
jobject JniJavaUtilArrayListCatchAll(JNIEnv *env);

jobject JniJavaUtilArrayListGlobalRef(JNIEnv *env);
jobject JniJavaUtilArrayListGlobalRefCatchAll(JNIEnv *env);

jboolean JniJavaUtilArrayListAdd(JNIEnv *env, jobject thiz, jobject object);
jboolean JniJavaUtilArrayListAddCatchAll(JNIEnv *env, jobject thiz,
                                         jobject object);

int JniLoadClassJavaUtilArrayList(JNIEnv *env);

#endif
