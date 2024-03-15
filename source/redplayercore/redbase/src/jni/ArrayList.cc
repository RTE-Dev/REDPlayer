#if defined(__ANDROID__)

#include "jni/ArrayList.h"
#include "jni/Util.h"

typedef struct JavaUtilArrayListFields {
  jclass id;

  jmethodID constructor_ArrayList;
  jmethodID method_add;
} JavaUtilArrayListFields;
static JavaUtilArrayListFields kFields;

static jobject JniJavaUtilArrayList(JNIEnv *env, bool catch_all) {
  jobject object = JniNewObject(env, kFields.id, kFields.constructor_ArrayList);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return object;
}

static jobject JniJavaUtilArrayListGlobalRef(JNIEnv *env, bool catch_all) {
  jobject ret_object =
      JniNewObjectGlobalRef(env, kFields.id, kFields.constructor_ArrayList);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret_object;
}

static jboolean JniJavaUtilArrayListAdd(JNIEnv *env, jobject thiz,
                                        jobject object, bool catch_all) {
  jboolean ret = env->CallBooleanMethod(thiz, kFields.method_add, object);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret;
}

jobject JniJavaUtilArrayList(JNIEnv *env) {
  return JniJavaUtilArrayList(env, false);
}
jobject JniJavaUtilArrayListCatchAll(JNIEnv *env) {
  return JniJavaUtilArrayList(env, true);
}

jobject JniJavaUtilArrayListGlobalRef(JNIEnv *env) {
  return JniJavaUtilArrayListGlobalRef(env, false);
}
jobject JniJavaUtilArrayListGlobalRefCatchAll(JNIEnv *env) {
  return JniJavaUtilArrayListGlobalRef(env, true);
}

jboolean JniJavaUtilArrayListAdd(JNIEnv *env, jobject thiz, jobject object) {
  return JniJavaUtilArrayListAdd(env, thiz, object, false);
}
jboolean JniJavaUtilArrayListAddCatchAll(JNIEnv *env, jobject thiz,
                                         jobject object) {
  return JniJavaUtilArrayListAdd(env, thiz, object, true);
}

int JniLoadClassJavaUtilArrayList(JNIEnv *env) {
  int ret = -1;
  const char *name = nullptr;
  const char *sign = nullptr;
  jclass class_id = nullptr;

  if (kFields.id != nullptr)
    return 0;

  sign = "java/util/ArrayList";
  kFields.id = JniGetClassGlobalRefCatchAll(env, sign);
  if (kFields.id == nullptr)
    return ret;

  class_id = kFields.id;
  name = "<init>";
  sign = "()V";
  kFields.constructor_ArrayList =
      JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (kFields.constructor_ArrayList == nullptr)
    return ret;

  class_id = kFields.id;
  name = "add";
  sign = "(Ljava/lang/Object;)Z";
  kFields.method_add = JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (kFields.method_add == nullptr)
    return ret;

  ret = 0;
  return ret;
}

#endif // __ANDROID__
