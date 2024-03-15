#if defined(__ANDROID__)

#include <new>
#include <stdexcept>
#include <sys/system_properties.h>

#include "RedDebug.h"
#include "jni/ArrayList.h"
#include "jni/Bundle.h"
#include "jni/ByteBuffer.h"
#include "jni/Debug.h"
#include "jni/Env.h"
#include "jni/Util.h"

static JavaVM *kJvm = nullptr;

//
/*
 * Get a human-readable summary of an exception object.  The buffer will
 * be populated with the "binary" class name and, if present, the
 * exception message.
 */
static void GetExceptionSummary(JNIEnv *env, jthrowable exception, char *buf,
                                size_t buf_len) {
  int success = 0;
  /* get the name of the exception's class */
  jclass exception_clazz = env->GetObjectClass(exception); // can't fail
  jclass class_clazz =
      env->GetObjectClass(exception_clazz); // java.lang.Class, can't fail
  jmethodID get_name_method =
      env->GetMethodID(class_clazz, "getName", "()Ljava/lang/String;");
  jstring class_name_str =
      (jstring)env->CallObjectMethod(exception_clazz, get_name_method);
  if (class_name_str != NULL) {
    /* get printable string */
    const char *class_name_chars = env->GetStringUTFChars(class_name_str, NULL);
    if (class_name_chars != NULL) {
      /* if the exception has a message string, get that */
      jmethodID get_message_method = env->GetMethodID(
          exception_clazz, "getMessage", "()Ljava/lang/String;");
      jstring message_str =
          (jstring)env->CallObjectMethod(exception, get_message_method);
      if (message_str != NULL) {
        const char *message_chars = env->GetStringUTFChars(message_str, NULL);
        if (message_chars != NULL) {
          snprintf(buf, buf_len, "%s: %s", class_name_chars, message_chars);
          env->ReleaseStringUTFChars(message_str, message_chars);
        } else {
          env->ExceptionClear(); // clear OOM
          snprintf(buf, buf_len, "%s: <error getting message>",
                   class_name_chars);
        }
        env->DeleteLocalRef(message_str);
      } else {
        strncpy(buf, class_name_chars, buf_len);
        buf[buf_len - 1] = '\0';
      }
      env->ReleaseStringUTFChars(class_name_str, class_name_chars);
      success = 1;
    }
    env->DeleteLocalRef(class_name_str);
  }
  env->DeleteLocalRef(class_clazz);
  env->DeleteLocalRef(exception_clazz);
  if (!success) {
    env->ExceptionClear();
    snprintf(buf, buf_len, "%s", "<error getting class name>");
  }
}
/*
 * Formats an exception as a string with its stack trace.
 */
static void PrintStackTrace(JNIEnv *env, jthrowable exception, char *buf,
                            size_t buf_len) {
  int success = 0;
  jclass string_writer_clazz = env->FindClass("java/io/StringWriter");
  if (string_writer_clazz != NULL) {
    jmethodID string_writer_ctor =
        env->GetMethodID(string_writer_clazz, "<init>", "()V");
    jmethodID to_string_method = env->GetMethodID(
        string_writer_clazz, "toString", "()Ljava/lang/String;");
    jclass print_writer_clazz = env->FindClass("java/io/PrintWriter");
    if (print_writer_clazz != NULL) {
      jmethodID print_writer_ctor =
          env->GetMethodID(print_writer_clazz, "<init>", "(Ljava/io/Writer;)V");
      jobject string_writer_obj =
          env->NewObject(string_writer_clazz, string_writer_ctor);
      if (string_writer_obj != NULL) {
        jobject print_writer_obj = env->NewObject(
            print_writer_clazz, print_writer_ctor, string_writer_obj);
        if (print_writer_obj != NULL) {
          jclass exception_clazz = env->GetObjectClass(exception); // can't fail
          jmethodID print_stack_trace_method = env->GetMethodID(
              exception_clazz, "PrintStackTrace", "(Ljava/io/PrintWriter;)V");
          env->CallVoidMethod(exception, print_stack_trace_method,
                              print_writer_obj);
          if (!env->ExceptionCheck()) {
            jstring message_str = (jstring)env->CallObjectMethod(
                string_writer_obj, to_string_method);
            if (message_str != NULL) {
              jsize message_str_length = env->GetStringLength(message_str);
              if (message_str_length >= (jsize)buf_len) {
                message_str_length = buf_len - 1;
              }
              env->GetStringUTFRegion(message_str, 0, message_str_length, buf);
              env->DeleteLocalRef(message_str);
              buf[message_str_length] = '\0';
              success = 1;
            }
          }
          env->DeleteLocalRef(exception_clazz);
          env->DeleteLocalRef(print_writer_obj);
        }
        env->DeleteLocalRef(string_writer_obj);
      }
      env->DeleteLocalRef(print_writer_clazz);
    }
    env->DeleteLocalRef(string_writer_clazz);
  }
  if (!success) {
    env->ExceptionClear();
    GetExceptionSummary(env, exception, buf, buf_len);
  }
}

static jobject JniNewGlobalRef(JNIEnv *env, jobject obj, bool catch_all) {
  jobject obj_global = env->NewGlobalRef(obj);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return obj_global;
}

static jobject JniNewObject(JNIEnv *env, jclass clazz, jmethodID method,
                            bool catch_all) {
  if (!clazz || !method) {
    return nullptr;
  }

  jobject object = env->NewObject(clazz, method);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return object;
}

static jobject JniNewObjectGlobalRef(JNIEnv *env, jclass clazz,
                                     jmethodID method, bool catch_all) {
  jobject object = JniNewObject(env, clazz, method);
  if (!object) {
    return object;
  }

  jobject object_global = JniNewGlobalRef(env, object, catch_all);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, &object);

  return object_global;
}

static jclass JniGetClass(JNIEnv *env, const char *name, bool catch_all) {
  name = (name ? name : "");

  jclass clazz = env->FindClass(name);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return clazz;
}

static jclass JniGetClassGlobalRef(JNIEnv *env, const char *name,
                                   bool catch_all) {
  jclass clazz = JniGetClass(env, name);
  if (!clazz) {
    return clazz;
  }

  jclass clazz_global = (jclass)JniNewGlobalRef(env, clazz, catch_all);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&clazz));

  return clazz_global;
}

static std::string JniGetClassName(JNIEnv *env, jclass clazz, bool catch_all) {
  if (!clazz)
    return "<null>";

  jmethodID method = env->GetMethodID(clazz, "getName", "()Ljava/lang/String;");
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  if (!method)
    return "???";

  jstring name = jstring(env->CallObjectMethod(clazz, method));
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&method));

  return JniGetStringUTFChars(env, name);
}

static std::string JniGetStringUTFChars(JNIEnv *env, jstring str,
                                        bool catch_all) {
  std::string result;

  if (!str)
    return result;

  const char *data = env->GetStringUTFChars(str, 0);

  result = (data ? data : "");

  JniReleaseStringUTFChars(env, str, data);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return result;
}

static jstring JniNewStringUTF(JNIEnv *env, const char *str, bool catch_all) {
  jstring result = env->NewStringUTF(str ? str : "");
  if (catch_all) {
    JniCheckAndLogException(env);
  }
  return result;
}

static void JniReleaseStringUTFChars(JNIEnv *env, jstring jstr, const char *str,
                                     bool catch_all) {
  if (!jstr || !str) {
    return;
  }
  env->ReleaseStringUTFChars(jstr, str);
  if (catch_all) {
    JniCheckAndLogException(env);
  }
}

static jmethodID JniGetStaticClassMethod(JNIEnv *env, jclass clazz,
                                         const char *name,
                                         const char *signature,
                                         bool catch_all) {
  CHECK(env != nullptr);
  CHECK(clazz != nullptr);

  name = (name ? name : "");
  signature = (signature ? signature : "");

  jmethodID method = env->GetStaticMethodID(clazz, name, signature);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return method;
}

static jmethodID JniGetClassMethod(JNIEnv *env, jclass clazz, const char *name,
                                   const char *signature, bool catch_all) {
  CHECK(env != nullptr);
  CHECK(clazz != nullptr);

  name = (name ? name : "");
  signature = (signature ? signature : "");

  jmethodID method = env->GetMethodID(clazz, name, signature);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return method;
}

static jmethodID JniGetObjectMethod(JNIEnv *env, jobject obj, const char *name,
                                    const char *signature, bool catch_all) {
  jclass clazz = env->GetObjectClass(obj);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  jmethodID method = JniGetClassMethod(env, clazz, name, signature, catch_all);

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&clazz));

  return method;
}

static jfieldID JniGetStaticFieldId(JNIEnv *env, jclass clazz, const char *name,
                                    const char *signature, bool catch_all) {
  CHECK(env != nullptr);
  CHECK(clazz != nullptr);

  name = (name ? name : "");
  signature = (signature ? signature : "");

  jfieldID fid = env->GetStaticFieldID(clazz, name, signature);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK2(fid != nullptr, "field:%s, sig:%s", name, signature);

  return fid;
}

static jfieldID JniGetFieldId(JNIEnv *env, jclass clazz, const char *name,
                              const char *signature, bool catch_all) {
  CHECK(env != nullptr);
  CHECK(clazz != nullptr);

  name = (name ? name : "");
  signature = (signature ? signature : "");

  jfieldID fid = env->GetFieldID(clazz, name, signature);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  CHECK2(fid != nullptr, "field:%s, sig:%s", name, signature);

  return fid;
}

static jbyteArray JniNewByteArray(JNIEnv *env, jsize capacity, bool catch_all) {
  CHECK_GT(capacity, 0);
  if (capacity <= 0) {
    return nullptr;
  }
  jbyteArray local = env->NewByteArray(capacity);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  return local;
}

static jbyteArray JniNewByteArrayGlobalRef(JNIEnv *env, jsize capacity,
                                           bool catch_all) {
  CHECK_GT(capacity, 0);
  if (capacity <= 0) {
    return nullptr;
  }
  jbyteArray local = env->NewByteArray(capacity);
  if (catch_all && JniCheckExceptionCatchAll(env)) {
    return nullptr;
  }
  CHECK(local != nullptr);
  if (!local) {
    return nullptr;
  }
  jbyteArray global =
      (jbyteArray)JniNewGlobalRef(env, (jobject)local, catch_all);
  if (catch_all) {
    JniCheckAndLogException(env);
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&local));

  return global;
}

// Interface start

int LoadAllClass(JNIEnv *env) {
  if (JniLoadClassAndroidOsBundle(env)) {
    return -1;
  }
  if (JniLoadClassJavaNioByteBuffer(env)) {
    return -1;
  }
  if (JniLoadClassJavaUtilArrayList(env)) {
    return -1;
  }
  return 0;
}

JavaVM *GetJVM() { return kJvm; }

int JniGetApiLevel() {
  int ret = 0;
  char value[1024] = {0};
  __system_property_get("ro.build.version.sdk", value);
  ret = atoi(value);
  return ret;
}

bool JniCheckException(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    return true;
  }

  return false;
}

bool JniCheckExceptionCatchAll(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
  }

  return false;
}

void JniLogException(JNIEnv *env) {
  jthrowable exception = env->ExceptionOccurred();
  if (!exception) {
    return;
  }
  env->ExceptionDescribe();
  env->ExceptionClear();

  char szbuffer[1024] = {0};
  PrintStackTrace(env, exception, szbuffer, sizeof(szbuffer));
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&exception));
  AV_LOGW("jniException", "%s\n", szbuffer);
}

void JniCheckAndLogException(JNIEnv *env) {
  if (JniCheckException(env)) {
    JniLogException(env);
  }
}

void JniTryCatch(JNIEnv *env, const std::function<void()> &body) {
  try {
    body();
  } catch (const std::bad_alloc &e) {
    JniSetException(env, JAVA_OUT_OF_MEMORY_ERROR, e.what());
  } catch (const std::exception &e) {
    JniSetException(env, JAVA_RUNTIME_EXCEPTION, e.what());
  } catch (...) {
    JniSetException(env, JAVA_RUNTIME_EXCEPTION,
                    "Unhandled exception in C++ code.");
  }
}

jint JniSetException(JNIEnv *env, const char *class_name,
                     const char *message) noexcept {
  class_name = (class_name ? class_name : "");
  message = (message ? message : "");
  jint ret = 0;

  AV_LOGW("JNI", "%s occured exception, %s\n", class_name, message);

  jclass clazz = env->FindClass(class_name);

  if (clazz) {
    ret = env->ThrowNew(clazz, message);
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&clazz));
    return ret;
  }

  ret =
      env->ThrowNew(env->FindClass(JAVA_CLASS_NOT_FOUND_EXCEPTION), class_name);
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&clazz));
  return ret;
}

jint JniSetException(JNIEnv *env, const char *class_name,
                     const std::string &message) noexcept {
  return JniSetException(env, class_name, message.c_str());
}

jobject JniNewGlobalRef(JNIEnv *env, jobject obj) {
  return JniNewGlobalRef(env, obj, false);
}

jobject JniNewGlobalRefCatchAll(JNIEnv *env, jobject obj) {
  return JniNewGlobalRef(env, obj, true);
}

jobject JniNewObject(JNIEnv *env, jclass clazz, jmethodID method) {
  return JniNewObject(env, clazz, method, false);
}

jobject JniNewObjectCatchAll(JNIEnv *env, jclass clazz, jmethodID method) {
  return JniNewObject(env, clazz, method, true);
}

jobject JniNewObjectGlobalRef(JNIEnv *env, jclass clazz, jmethodID method) {
  return JniNewObjectGlobalRef(env, clazz, method, false);
}

jobject JniNewObjectGlobalRefCatchAll(JNIEnv *env, jclass clazz,
                                      jmethodID method) {
  return JniNewObjectGlobalRef(env, clazz, method, true);
}

jclass JniGetClass(JNIEnv *env, const char *name) {
  return JniGetClass(env, name, false);
}

jclass JniGetClassCatchAll(JNIEnv *env, const char *name) {
  return JniGetClass(env, name, true);
}

jclass JniGetClass(JNIEnv *env, const std::string &name) {
  return JniGetClass(env, name.c_str());
}

jclass JniGetClassCatchAll(JNIEnv *env, const std::string &name) {
  return JniGetClassCatchAll(env, name.c_str());
}

jclass JniGetClassGlobalRef(JNIEnv *env, const char *name) {
  return JniGetClassGlobalRef(env, name, false);
}

jclass JniGetClassGlobalRefCatchAll(JNIEnv *env, const char *name) {
  return JniGetClassGlobalRef(env, name, true);
}

jclass JniGetClassGlobalRef(JNIEnv *env, const std::string &name) {
  return JniGetClassGlobalRef(env, name.c_str());
}

jclass JniGetClassGlobalRefCatchAll(JNIEnv *env, const std::string &name) {
  return JniGetClassGlobalRefCatchAll(env, name.c_str());
}

std::string JniGetClassName(JNIEnv *env, jclass clazz) {
  return JniGetClassName(env, clazz, false);
}

std::string JniGetClassNameCatchAll(JNIEnv *env, jclass clazz) {
  return JniGetClassName(env, clazz, true);
}

std::string JniGetStringUTFChars(JNIEnv *env, jstring str) {
  return JniGetStringUTFChars(env, str, false);
}

std::string JniGetStringUTFCharsCatchAll(JNIEnv *env, jstring str) {
  return JniGetStringUTFChars(env, str, true);
}

jstring JniNewStringUTF(JNIEnv *env, const char *str) {
  return JniNewStringUTF(env, str, false);
}

jstring JniNewStringUTFCatchAll(JNIEnv *env, const char *str) {
  return JniNewStringUTF(env, str, true);
}

jstring JniNewStringUTF(JNIEnv *env, const std::string &str) {
  return JniNewStringUTF(env, str.c_str());
}

jstring JniNewStringUTFCatchAll(JNIEnv *env, const std::string &str) {
  return JniNewStringUTFCatchAll(env, str.c_str());
}

void JniReleaseStringUTFChars(JNIEnv *env, jstring jstr, const char *str) {
  return JniReleaseStringUTFChars(env, jstr, str, false);
}

void JniReleaseStringUTFCharsCatchAll(JNIEnv *env, jstring jstr,
                                      const char *str) {
  return JniReleaseStringUTFChars(env, jstr, str, true);
}

jmethodID JniGetStaticClassMethod(JNIEnv *env, jclass clazz, const char *name,
                                  const char *signature) {
  return JniGetStaticClassMethod(env, clazz, name, signature, false);
}

jmethodID JniGetStaticClassMethodCatchAll(JNIEnv *env, jclass clazz,
                                          const char *name,
                                          const char *signature) {
  return JniGetStaticClassMethod(env, clazz, name, signature, true);
}

jmethodID JniGetClassMethod(JNIEnv *env, jclass clazz, const char *name,
                            const char *signature) {
  return JniGetClassMethod(env, clazz, name, signature, false);
}

jmethodID JniGetClassMethodCatchAll(JNIEnv *env, jclass clazz, const char *name,
                                    const char *signature) {
  return JniGetClassMethod(env, clazz, name, signature, true);
}

jmethodID JniGetObjectMethod(JNIEnv *env, jobject obj, const char *name,
                             const char *signature) {
  return JniGetObjectMethod(env, obj, name, signature, false);
}

jmethodID JniGetObjectMethodCatchAll(JNIEnv *env, jobject obj, const char *name,
                                     const char *signature) {
  return JniGetObjectMethod(env, obj, name, signature, true);
}

jfieldID JniGetStaticFieldId(JNIEnv *env, jclass clazz, const char *name,
                             const char *signature) {
  return JniGetStaticFieldId(env, clazz, name, signature, false);
}

jfieldID JniGetStaticFieldIdCatchAll(JNIEnv *env, jclass clazz,
                                     const char *name, const char *signature) {
  return JniGetStaticFieldId(env, clazz, name, signature, true);
}

jfieldID JniGetFieldId(JNIEnv *env, jclass clazz, const char *name,
                       const char *signature) {
  return JniGetFieldId(env, clazz, name, signature, false);
}

jfieldID JniGetFieldIdCatchAll(JNIEnv *env, jclass clazz, const char *name,
                               const char *signature) {
  return JniGetFieldId(env, clazz, name, signature, true);
}

jbyteArray JniNewByteArray(JNIEnv *env, jsize capacity) {
  return JniNewByteArray(env, capacity, false);
}

jbyteArray JniNewByteArrayCatchAll(JNIEnv *env, jsize capacity) {
  return JniNewByteArray(env, capacity, true);
}

jbyteArray JniNewByteArrayGlobalRef(JNIEnv *env, jsize capacity) {
  return JniNewByteArrayGlobalRef(env, capacity, false);
}

jbyteArray JniNewByteArrayGlobalRefCatchAll(JNIEnv *env, jsize capacity) {
  return JniNewByteArrayGlobalRef(env, capacity, true);
}

void JniDeleteLocalRef(JNIEnv *env, jobject obj) {
  if (!obj)
    return;
  env->DeleteLocalRef(obj);
}

void JniDeleteLocalRefP(JNIEnv *env, jobject *obj) {
  if (!obj)
    return;
  JniDeleteLocalRef(env, *obj);
  *obj = nullptr;
}

void JniDeleteGlobalRef(JNIEnv *env, jobject obj) {
  if (!obj)
    return;
  env->DeleteGlobalRef(obj);
}

void JniDeleteGlobalRefP(JNIEnv *env, jobject *obj) {
  if (!obj)
    return;
  JniDeleteGlobalRef(env, *obj);
  *obj = nullptr;
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *jvm, void *reserved) {
  int ret = 0;
  JNIEnv *env = nullptr;
  if (JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
    return JNI_ERR;
  }

  jni::JniEnvPtr::GlobalInit(jvm);
  kJvm = jvm;

  ret = LoadAllClass(env);
  JNI_CHECK_RET(ret == 0, env, nullptr, nullptr, -1);

  return JNI_VERSION_1_6;
}

#endif // __ANDROID__
