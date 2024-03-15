#if defined(__ANDROID__)

#include "jni/ByteBuffer.h"
#include "RedDebug.h"
#include "jni/Util.h"

typedef struct JavaNioByteBuffer {
  jclass id;

  jmethodID method_allocate;
  jmethodID method_allocateDirect;
  jmethodID method_limit;
} JavaNioByteBuffer;
static JavaNioByteBuffer kFields;

static jobject JniJavaNioByteBufferAllocate(JNIEnv *env, jint capacity,
                                            bool catch_all) {
  jobject obj = env->CallStaticObjectMethod(kFields.id, kFields.method_allocate,
                                            capacity);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK(obj);

  return obj;
}

static jobject JniJavaNioByteBufferAllocateGlobalRef(JNIEnv *env, jint capacity,
                                                     bool catch_all) {
  jobject ret_object = nullptr;
  jobject local_object = JniJavaNioByteBufferAllocate(env, capacity, catch_all);

  CHECK(local_object);

  if (!local_object) {
    return ret_object;
  }

  ret_object = JniNewGlobalRef(env, local_object);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK(ret_object);

  JniDeleteLocalRefP(env, &local_object);
  return ret_object;
}

static jobject JniJavaNioByteBufferAllocateDirect(JNIEnv *env, jint capacity,
                                                  bool catch_all) {
  jobject obj = env->CallStaticObjectMethod(
      kFields.id, kFields.method_allocateDirect, capacity);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK(obj);

  return obj;
}

static jobject JniJavaNioByteBufferAllocateDirectGlobalRef(JNIEnv *env,
                                                           jint capacity,
                                                           bool catch_all) {
  jobject ret_object = nullptr;
  jobject local_object =
      JniJavaNioByteBufferAllocateDirect(env, capacity, catch_all);

  CHECK(local_object);

  if (!local_object) {
    return ret_object;
  }

  ret_object = JniNewGlobalRef(env, local_object);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, &local_object);
  return ret_object;
}

static jobject JniJavaNioByteBufferLimit(JNIEnv *env, jobject thiz,
                                         jint new_limit, bool catch_all) {
  jobject obj = env->CallObjectMethod(thiz, kFields.method_limit, new_limit);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK(obj);

  return obj;
}

static jobject JniJavaNioByteBufferLimitGlobalRef(JNIEnv *env, jobject thiz,
                                                  jint new_limit,
                                                  bool catch_all) {
  jobject ret_object = nullptr;
  jobject local_object =
      JniJavaNioByteBufferLimit(env, thiz, new_limit, catch_all);

  CHECK(local_object);

  if (!local_object) {
    return ret_object;
  }

  ret_object = JniNewGlobalRef(env, local_object);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, &local_object);
  return ret_object;
}

jobject JniJavaNioByteBufferAllocate(JNIEnv *env, jint capacity) {
  return JniJavaNioByteBufferAllocate(env, capacity, false);
}
jobject JniJavaNioByteBufferAllocateCatchAll(JNIEnv *env, jint capacity) {
  return JniJavaNioByteBufferAllocate(env, capacity, true);
}
jobject JniJavaNioByteBufferAllocateGlobalRef(JNIEnv *env, jint capacity) {
  return JniJavaNioByteBufferAllocateGlobalRef(env, capacity, false);
}
jobject JniJavaNioByteBufferAllocateGlobalRefCatchAll(JNIEnv *env,
                                                      jint capacity) {
  return JniJavaNioByteBufferAllocateGlobalRef(env, capacity, true);
}
jobject JniJavaNioByteBufferAllocateDirect(JNIEnv *env, jint capacity) {
  return JniJavaNioByteBufferAllocateDirect(env, capacity, false);
}
jobject JniJavaNioByteBufferAllocateDirectCatchAll(JNIEnv *env, jint capacity) {
  return JniJavaNioByteBufferAllocateDirect(env, capacity, true);
}
jobject JniJavaNioByteBufferAllocateDirectGlobalRef(JNIEnv *env,
                                                    jint capacity) {
  return JniJavaNioByteBufferAllocateDirectGlobalRef(env, capacity, false);
}
jobject JniJavaNioByteBufferAllocateDirectGlobalRefCatchAll(JNIEnv *env,
                                                            jint capacity) {
  return JniJavaNioByteBufferAllocateDirectGlobalRef(env, capacity, true);
}
jobject JniJavaNioByteBufferLimit(JNIEnv *env, jobject thiz, jint new_limit) {
  return JniJavaNioByteBufferLimit(env, thiz, new_limit, false);
}
jobject JniJavaNioByteBufferLimitCatchAll(JNIEnv *env, jobject thiz,
                                          jint new_limit) {
  return JniJavaNioByteBufferLimit(env, thiz, new_limit, true);
}
jobject JniJavaNioByteBufferLimitGlobalRef(JNIEnv *env, jobject thiz,
                                           jint new_limit) {
  return JniJavaNioByteBufferLimitGlobalRef(env, thiz, new_limit, false);
}
jobject JniJavaNioByteBufferLimitGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                   jint new_limit) {
  return JniJavaNioByteBufferLimitGlobalRef(env, thiz, new_limit, true);
}

int JniLoadClassJavaNioByteBuffer(JNIEnv *env) {
  int ret = -1;
  const char *name = nullptr;
  const char *sign = nullptr;
  jclass class_id = nullptr;

  if (kFields.id != nullptr)
    return 0;

  sign = "java/nio/ByteBuffer";
  kFields.id = JniGetClassGlobalRefCatchAll(env, sign);
  if (kFields.id == nullptr)
    return ret;

  class_id = kFields.id;
  name = "allocate";
  sign = "(I)Ljava/nio/ByteBuffer;";
  kFields.method_allocate =
      JniGetStaticClassMethodCatchAll(env, class_id, name, sign);

  CHECK(kFields.method_allocate);

  if (kFields.method_allocate == nullptr)
    return ret;

  class_id = kFields.id;
  name = "allocateDirect";
  sign = "(I)Ljava/nio/ByteBuffer;";
  kFields.method_allocateDirect =
      JniGetStaticClassMethodCatchAll(env, class_id, name, sign);

  CHECK(kFields.method_allocateDirect);

  if (kFields.method_allocateDirect == nullptr)
    return ret;

  class_id = kFields.id;
  name = "limit";
  sign = "(I)Ljava/nio/Buffer;";
  kFields.method_limit = JniGetClassMethodCatchAll(env, class_id, name, sign);

  CHECK(kFields.method_limit);

  if (kFields.method_limit == nullptr)
    return ret;

  ret = 0;
  return ret;
}

#endif // __ANDROID__
