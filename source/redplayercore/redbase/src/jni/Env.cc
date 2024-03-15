#if defined(__ANDROID__)
#include <pthread.h>

#include "jni/Env.h"

static pthread_key_t kThreadKey;
JavaVM *kJvm = nullptr;

jni::JniEnvPtr::JniEnvPtr() {
  if (!kJvm) {
    return;
  }

  if (kJvm->GetEnv(reinterpret_cast<void **>(&env_), JNI_VERSION_1_6) !=
      JNI_EDETACHED) {
    return;
  }

  if (kJvm->AttachCurrentThread(&env_, nullptr) < 0) {
    return;
  }

  pthread_setspecific(kThreadKey, env_);
}

jni::JniEnvPtr::~JniEnvPtr() {}

JNIEnv *jni::JniEnvPtr::operator->() { return env_; }

JNIEnv *jni::JniEnvPtr::env() const { return env_; }

void jni::JniEnvPtr::JniThreadDestroyed(void *value) {
  JNIEnv *env = reinterpret_cast<JNIEnv *>(value);

  if (env != nullptr && kJvm != nullptr) {
    kJvm->DetachCurrentThread();
    pthread_setspecific(kThreadKey, nullptr);
  }
}

void jni::JniEnvPtr::GlobalInit(JavaVM *vm) {
  kJvm = vm;
  pthread_key_create(&kThreadKey, JniThreadDestroyed);
}

#endif // __ANDROID__
