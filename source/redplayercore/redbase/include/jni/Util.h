#pragma once

#if defined(__ANDROID__)

#include <functional>
#include <jni.h>
#include <string>

#define JAVA_ILLEGAL_ARGUMENT_EXCEPTION "java/lang/IllegalArgumentException"
#define JAVA_CLASS_NOT_FOUND_EXCEPTION "java/lang/ClassNotFoundException"
#define JAVA_ILLEGAL_STATE_EXCEPTION "java/lang/IllegalStateException"
#define JAVA_RUNTIME_EXCEPTION "java/lang/RuntimeException"
#define JAVA_INTERNAL_ERROR "java/lang/InternalError"
#define JAVA_OUT_OF_MEMORY_ERROR "java/lang/OutOfMemoryError"

JavaVM *GetJVM();
int JniGetApiLevel();
bool JniCheckException(JNIEnv *env);
bool JniCheckExceptionCatchAll(JNIEnv *env);
void JniLogException(JNIEnv *env);
void JniCheckAndLogException(JNIEnv *env);
void JniTryCatch(JNIEnv *env, const std::function<void()> &body);
jint JniSetException(JNIEnv *env, const char *class_name,
                     const char *message) noexcept;
jint JniSetException(JNIEnv *env, const char *class_name,
                     const std::string &message) noexcept;

jobject JniNewGlobalRef(JNIEnv *env, jobject obj);
jobject JniNewGlobalRefCatchAll(JNIEnv *env, jobject obj);
jobject JniNewObject(JNIEnv *env, jclass clazz, jmethodID method);
jobject JniNewObjectCatchAll(JNIEnv *env, jclass clazz, jmethodID method);
jobject JniNewObjectGlobalRef(JNIEnv *env, jclass clazz, jmethodID method);
jobject JniNewObjectGlobalRefCatchAll(JNIEnv *env, jclass clazz,
                                      jmethodID method);

jclass JniGetClass(JNIEnv *env, const char *name);
jclass JniGetClassCatchAll(JNIEnv *env, const char *name);
jclass JniGetClass(JNIEnv *env, const std::string &name);
jclass JniGetClassCatchAll(JNIEnv *env, const std::string &name);
jclass JniGetClassGlobalRef(JNIEnv *env, const char *name);
jclass JniGetClassGlobalRefCatchAll(JNIEnv *env, const char *name);
jclass JniGetClassGlobalRef(JNIEnv *env, const std::string &name);
jclass JniGetClassGlobalRefCatchAll(JNIEnv *env, const std::string &name);

std::string JniGetClassName(JNIEnv *env, jclass clazz);
std::string JniGetClassNameCatchAll(JNIEnv *env, jclass clazz);

std::string JniGetStringUTFChars(JNIEnv *env, jstring str);
std::string JniGetStringUTFCharsCatchAll(JNIEnv *env, jstring str);
jstring JniNewStringUTF(JNIEnv *env, const char *str);
jstring JniNewStringUTFCatchAll(JNIEnv *env, const char *str);
jstring JniNewStringUTF(JNIEnv *env, const std::string &str);
jstring JniNewStringUTFCatchAll(JNIEnv *env, const std::string &str);
void JniReleaseStringUTFChars(JNIEnv *env, jstring jstr, const char *str);
void JniReleaseStringUTFCharsCatchAll(JNIEnv *env, jstring jstr,
                                      const char *str);

jmethodID JniGetStaticClassMethod(JNIEnv *env, jclass clazz, const char *name,
                                  const char *signature);
jmethodID JniGetStaticClassMethodCatchAll(JNIEnv *env, jclass clazz,
                                          const char *name,
                                          const char *signature);
jmethodID JniGetClassMethod(JNIEnv *env, jclass clazz, const char *name,
                            const char *signature);
jmethodID JniGetClassMethodCatchAll(JNIEnv *env, jclass clazz, const char *name,
                                    const char *signature);
jmethodID JniGetObjectMethod(JNIEnv *env, jobject obj, const char *name,
                             const char *signature);
jmethodID JniGetObjectMethodCatchAll(JNIEnv *env, jobject obj, const char *name,
                                     const char *signature);

jfieldID JniGetStaticFieldId(JNIEnv *env, jclass clazz, const char *name,
                             const char *signature);
jfieldID JniGetStaticFieldIdCatchAll(JNIEnv *env, jclass clazz,
                                     const char *name, const char *signature);
jfieldID JniGetFieldId(JNIEnv *env, jclass clazz, const char *name,
                       const char *signature);
jfieldID JniGetFieldIdCatchAll(JNIEnv *env, jclass clazz, const char *name,
                               const char *signature);

jbyteArray JniNewByteArray(JNIEnv *env, jsize capacity);
jbyteArray JniNewByteArrayCatchAll(JNIEnv *env, jsize capacity);
jbyteArray JniNewByteArrayGlobalRef(JNIEnv *env, jsize capacity);
jbyteArray JniNewByteArrayGlobalRefCatchAll(JNIEnv *env, jsize capacity);

void JniDeleteLocalRef(JNIEnv *env, jobject obj);
void JniDeleteLocalRefP(JNIEnv *env, jobject *obj);
void JniDeleteGlobalRef(JNIEnv *env, jobject obj);
void JniDeleteGlobalRefP(JNIEnv *env, jobject *obj);

#endif // __ANDROID__
