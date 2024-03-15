#if defined(__ANDROID__)

#include "jni/Bundle.h"
#include "jni/Util.h"

typedef struct AndroidOsBundleFields {
  jclass id;

  jmethodID constructor_Bundle;
  jmethodID method_getInt;
  jmethodID method_putInt;
  jmethodID method_getString;
  jmethodID method_putString;
  jmethodID method_putParcelableArrayList;
  jmethodID method_getLong;
  jmethodID method_putLong;
} AndroidOsBundleFields;
static AndroidOsBundleFields kFields;

static jobject JniAndroidOsBundle(JNIEnv *env, bool catch_all) {
  jobject object = JniNewObject(env, kFields.id, kFields.constructor_Bundle);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return object;
}

static jobject JniAndroidOsBundleGlobalRef(JNIEnv *env, bool catch_all) {
  jobject ret_object =
      JniNewObjectGlobalRef(env, kFields.id, kFields.constructor_Bundle);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret_object;
}

static jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, jstring key,
                                     jint default_value, bool catch_all) {
  jint ret =
      env->CallIntMethod(thiz, kFields.method_getInt, key, default_value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret;
}

static jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, const char *key,
                                     jint default_value, bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  jint ret =
      JniAndroidOsBundleGetInt(env, thiz, jkey, default_value, catch_all);
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  return ret;
}

static void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, jstring key,
                                     jint value, bool catch_all) {
  env->CallVoidMethod(thiz, kFields.method_putInt, key, value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
}

static void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, const char *key,
                                     jint value, bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniAndroidOsBundlePutInt(env, thiz, jkey, value, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
}

static jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz,
                                           jstring key, bool catch_all) {
  jstring ret =
      (jstring)env->CallObjectMethod(thiz, kFields.method_getString, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret;
}

static jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                                    jstring key,
                                                    bool catch_all) {
  jstring ret_object = nullptr;
  jstring local_object = JniAndroidOsBundleGetString(env, thiz, key, catch_all);

  if (!local_object)
    return ret_object;

  ret_object = (jstring)JniNewGlobalRef(env, (jobject)local_object);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&local_object));

  return ret_object;
}

static std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                                jstring key, bool catch_all) {
  jstring str = JniAndroidOsBundleGetString(env, thiz, key, catch_all);
  std::string ret = JniGetStringUTFChars(env, str);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&str));

  return ret;
}

static jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz,
                                           const char *key, bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  jstring ret = JniAndroidOsBundleGetString(env, thiz, jkey, catch_all);
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  return ret;
}

static jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                                    const char *key,
                                                    bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  jstring ret =
      JniAndroidOsBundleGetStringGlobalRef(env, thiz, jkey, catch_all);
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  return ret;
}

static std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                                const char *key,
                                                bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  jstring str = JniAndroidOsBundleGetString(env, thiz, jkey, catch_all);
  std::string ret = JniGetStringUTFChars(env, str);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&str));

  return ret;
}

static void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz, jstring key,
                                        jstring value, bool catch_all) {
  env->CallVoidMethod(thiz, kFields.method_putString, key, value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
}

static void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz,
                                        const char *key, const char *value,
                                        bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  jstring jvalue = JniNewStringUTF(env, value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniAndroidOsBundlePutString(env, thiz, jkey, jvalue, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jvalue));
}

static void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                                     jstring key, jobject value,
                                                     bool catch_all) {
  env->CallVoidMethod(thiz, kFields.method_putParcelableArrayList, key, value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
}

static void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                                     const char *key,
                                                     jobject value,
                                                     bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniAndroidOsBundlePutParcelableArrayList(env, thiz, jkey, value, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
}

static jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz, jstring key,
                                       bool catch_all) {
  jlong ret = env->CallLongMethod(thiz, kFields.method_getLong, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return ret;
}

static jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz,
                                       const char *key, bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  jlong ret = JniAndroidOsBundleGetLong(env, thiz, jkey, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
  return ret;
}

static void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz, jstring key,
                                      jlong value, bool catch_all) {
  env->CallVoidMethod(thiz, kFields.method_putLong, key, value);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
}

static void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz,
                                      const char *key, jlong value,
                                      bool catch_all) {
  jstring jkey = JniNewStringUTF(env, key);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniAndroidOsBundlePutLong(env, thiz, jkey, value, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jkey));
}

jobject JniAndroidOsBundle(JNIEnv *env) {
  return JniAndroidOsBundle(env, false);
}
jobject JniAndroidOsBundleCatchAll(JNIEnv *env) {
  return JniAndroidOsBundle(env, true);
}

jobject JniAndroidOsBundleGlobalRef(JNIEnv *env) {
  return JniAndroidOsBundleGlobalRef(env, false);
}
jobject JniAndroidOsBundleGlobalRefCatchAll(JNIEnv *env) {
  return JniAndroidOsBundleGlobalRef(env, true);
}

jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, jstring key,
                              jint default_value) {
  return JniAndroidOsBundleGetInt(env, thiz, key, default_value, false);
}
jint JniAndroidOsBundleGetIntCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                      jint default_value) {
  return JniAndroidOsBundleGetInt(env, thiz, key, default_value, true);
}

jint JniAndroidOsBundleGetInt(JNIEnv *env, jobject thiz, const char *key,
                              jint default_value) {
  return JniAndroidOsBundleGetInt(env, thiz, key, default_value, false);
}
jint JniAndroidOsBundleGetIntCatchAll(JNIEnv *env, jobject thiz,
                                      const char *key, jint default_value) {
  return JniAndroidOsBundleGetInt(env, thiz, key, default_value, true);
}

void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, jstring key,
                              jint value) {
  return JniAndroidOsBundlePutInt(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutIntCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                      jint value) {
  return JniAndroidOsBundlePutInt(env, thiz, key, value, true);
}

void JniAndroidOsBundlePutInt(JNIEnv *env, jobject thiz, const char *key,
                              jint value) {
  return JniAndroidOsBundlePutInt(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutIntCatchAll(JNIEnv *env, jobject thiz,
                                      const char *key, jint value) {
  return JniAndroidOsBundlePutInt(env, thiz, key, value, true);
}

jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz, jstring key) {
  return JniAndroidOsBundleGetString(env, thiz, key, false);
}
jstring JniAndroidOsBundleGetStringCatchAll(JNIEnv *env, jobject thiz,
                                            jstring key) {
  return JniAndroidOsBundleGetString(env, thiz, key, true);
}

jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                             jstring key) {
  return JniAndroidOsBundleGetStringGlobalRef(env, thiz, key, false);
}
jstring JniAndroidOsBundleGetStringGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                     jstring key) {
  return JniAndroidOsBundleGetStringGlobalRef(env, thiz, key, true);
}

std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                         jstring key) {
  return JniAndroidOsBundleGetCString(env, thiz, key, false);
}
std::string JniAndroidOsBundleGetCStringCatchAll(JNIEnv *env, jobject thiz,
                                                 jstring key) {
  return JniAndroidOsBundleGetCString(env, thiz, key, true);
}

jstring JniAndroidOsBundleGetString(JNIEnv *env, jobject thiz,
                                    const char *key) {
  return JniAndroidOsBundleGetString(env, thiz, key, false);
}
jstring JniAndroidOsBundleGetStringCatchAll(JNIEnv *env, jobject thiz,
                                            const char *key) {
  return JniAndroidOsBundleGetString(env, thiz, key, true);
}

jstring JniAndroidOsBundleGetStringGlobalRef(JNIEnv *env, jobject thiz,
                                             const char *key) {
  return JniAndroidOsBundleGetStringGlobalRef(env, thiz, key, false);
}
jstring JniAndroidOsBundleGetStringGlobalRefCatchAll(JNIEnv *env, jobject thiz,
                                                     const char *key) {
  return JniAndroidOsBundleGetStringGlobalRef(env, thiz, key, true);
}

std::string JniAndroidOsBundleGetCString(JNIEnv *env, jobject thiz,
                                         const char *key) {
  return JniAndroidOsBundleGetCString(env, thiz, key, false);
}
std::string JniAndroidOsBundleGetCStringCatchAll(JNIEnv *env, jobject thiz,
                                                 const char *key) {
  return JniAndroidOsBundleGetCString(env, thiz, key, true);
}

void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz, jstring key,
                                 jstring value) {
  return JniAndroidOsBundlePutString(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutStringCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                         jstring value) {
  return JniAndroidOsBundlePutString(env, thiz, key, value, true);
}

void JniAndroidOsBundlePutString(JNIEnv *env, jobject thiz, const char *key,
                                 const char *value) {
  return JniAndroidOsBundlePutString(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutStringCatchAll(JNIEnv *env, jobject thiz,
                                         const char *key, const char *value) {
  return JniAndroidOsBundlePutString(env, thiz, key, value, true);
}

void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                              jstring key, jobject value) {
  return JniAndroidOsBundlePutParcelableArrayList(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutParcelableArrayListCatchAll(JNIEnv *env, jobject thiz,
                                                      jstring key,
                                                      jobject value) {
  return JniAndroidOsBundlePutParcelableArrayList(env, thiz, key, value, true);
}

void JniAndroidOsBundlePutParcelableArrayList(JNIEnv *env, jobject thiz,
                                              const char *key, jobject value) {
  return JniAndroidOsBundlePutParcelableArrayList(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutParcelableArrayListCatchAll(JNIEnv *env, jobject thiz,
                                                      const char *key,
                                                      jobject value) {
  return JniAndroidOsBundlePutParcelableArrayList(env, thiz, key, value, true);
}

jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz, jstring key) {
  return JniAndroidOsBundleGetLong(env, thiz, key, false);
}
jlong JniAndroidOsBundleGetLongCatchAll(JNIEnv *env, jobject thiz,
                                        jstring key) {
  return JniAndroidOsBundleGetLong(env, thiz, key, true);
}

jlong JniAndroidOsBundleGetLong(JNIEnv *env, jobject thiz, const char *key) {
  return JniAndroidOsBundleGetLong(env, thiz, key, false);
}
jlong JniAndroidOsBundleGetLongCatchAll(JNIEnv *env, jobject thiz,
                                        const char *key) {
  return JniAndroidOsBundleGetLong(env, thiz, key, true);
}

void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz, jstring key,
                               jlong value) {
  return JniAndroidOsBundlePutLong(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutLongCatchAll(JNIEnv *env, jobject thiz, jstring key,
                                       jlong value) {
  return JniAndroidOsBundlePutLong(env, thiz, key, value, true);
}

void JniAndroidOsBundlePutLong(JNIEnv *env, jobject thiz, const char *key,
                               jlong value) {
  return JniAndroidOsBundlePutLong(env, thiz, key, value, false);
}
void JniAndroidOsBundlePutLongCatchAll(JNIEnv *env, jobject thiz,
                                       const char *key, jlong value) {
  return JniAndroidOsBundlePutLong(env, thiz, key, value, true);
}

int JniLoadClassAndroidOsBundle(JNIEnv *env) {
  int ret = -1;
  const char *name = nullptr;
  const char *sign = nullptr;
  jclass class_id = nullptr;

  if (kFields.id)
    return 0;

  sign = "android/os/Bundle";
  kFields.id = JniGetClassGlobalRefCatchAll(env, sign);
  if (!kFields.id)
    return ret;

  class_id = kFields.id;
  name = "<init>";
  sign = "()V";
  kFields.constructor_Bundle =
      JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.constructor_Bundle)
    return ret;

  class_id = kFields.id;
  name = "getInt";
  sign = "(Ljava/lang/String;I)I";
  kFields.method_getInt = JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_getInt)
    return ret;

  class_id = kFields.id;
  name = "putInt";
  sign = "(Ljava/lang/String;I)V";
  kFields.method_putInt = JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_putInt)
    return ret;

  class_id = kFields.id;
  name = "getString";
  sign = "(Ljava/lang/String;)Ljava/lang/String;";
  kFields.method_getString =
      JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_getString)
    return ret;

  class_id = kFields.id;
  name = "putString";
  sign = "(Ljava/lang/String;Ljava/lang/String;)V";
  kFields.method_putString =
      JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_putString)
    return ret;

  class_id = kFields.id;
  name = "putParcelableArrayList";
  sign = "(Ljava/lang/String;Ljava/util/ArrayList;)V";
  kFields.method_putParcelableArrayList =
      JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_putParcelableArrayList)
    return ret;

  class_id = kFields.id;
  name = "getLong";
  sign = "(Ljava/lang/String;)J";
  kFields.method_getLong = JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_getLong)
    return ret;

  class_id = kFields.id;
  name = "putLong";
  sign = "(Ljava/lang/String;J)V";
  kFields.method_putLong = JniGetClassMethodCatchAll(env, class_id, name, sign);
  if (!kFields.method_putLong)
    return ret;

  ret = 0;
  return ret;
}

#endif // __ANDROID__
