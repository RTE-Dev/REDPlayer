#pragma once

#if defined(__ANDROID__)

#include <jni.h>
#include <string>

jobject JniAndroidOsBundle(JNIEnv *env);
jobject JniAndroidOsBundleCatchAll(JNIEnv *env);

jobject JniAndroidOsBundleGlobalRef(JNIEnv *env);
jobject JniAndroidOsBundleGlobalRefCatchAll(JNIEnv *env);

jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, jstring key,
                              jint default_value);
jint JniAndroidOsBundleGetIntCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                      jint default_value);

jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, const char *key,
                              jint default_value);
jint JniAndroidOsBundleGetIntCatchAll(JNIEnv *env, jobject thiz,
                                      const char *key, jint default_value);

void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, jstring key,
                              jint value);
void JniAndroidOsBundlePutIntCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                      jint value);

void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, const char *key,
                              jint value);
void JniAndroidOsBundlePutIntCatchAll(JNIEnv *env, jobject thiz,
                                      const char *key, jint value);

jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz, jstring key);
jstring JniAndroidOsBundleGetStringCatchAll(JNIEnv *env, jobject thiz,
                                            jstring key);

jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                             jstring key);
jstring JniAndroidOsBundleGetStringGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                     jstring key);

std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                         jstring key);
std::string JniAndroidOsBundleGetCStringCatchAll(JNIEnv *env, jobject thiz,
                                                 jstring key);

jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz, const char *key);
jstring JniAndroidOsBundleGetStringCatchAll(JNIEnv *env, jobject thiz,
                                            const char *key);

jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                             const char *key);
jstring JniAndroidOsBundleGetStringGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                     const char *key);

std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                         const char *key);
std::string JniAndroidOsBundleGetCStringCatchAll(JNIEnv *env, jobject thiz,
                                                 const char *key);

void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz, jstring key,
                                 jstring value);
void JniAndroidOsBundlePutStringCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                         jstring value);

void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz, const char *key,
                                 const char *value);
void JniAndroidOsBundlePutStringCatchAll(JNIEnv *env, jobject thiz,
                                         const char *key, const char *value);

void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                              jstring key, jobject value);
void JniAndroidOsBundlePutParcelableArrayListCatchAll(JNIEnv *env, jobject thiz,
                                                      jstring key,
                                                      jobject value);

void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                              const char *key, jobject value);
void JniAndroidOsBundlePutParcelableArrayListCatchAll(JNIEnv *env, jobject thiz,
                                                      const char *key,
                                                      jobject value);

jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz, jstring key);
jlong JniAndroidOsBundleGetLongCatchAll(JNIEnv *env, jobject thiz, jstring key);

jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz, const char *key);
jlong JniAndroidOsBundleGetLongCatchAll(JNIEnv *env, jobject thiz,
                                        const char *key);

void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz, jstring key,
                               jlong value);
void JniAndroidOsBundlePutLongCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                       jlong value);

void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz, const char *key,
                               jlong value);
void JniAndroidOsBundlePutLongCatchAll(JNIEnv *env, jobject thiz,
                                       const char *key, jlong value);

int JniLoadClassAndroidOsBundle(JNIEnv *env);

#endif
