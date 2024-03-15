#pragma once

#if defined(__ANDROID__)

#include <jni.h>

namespace jni {

class JniEnvPtr {
public:
  JniEnvPtr();
  ~JniEnvPtr();
  JNIEnv *operator->();
  JNIEnv *env() const;
  static void GlobalInit(JavaVM *vm);

private:
  JniEnvPtr(const JniEnvPtr &) = delete;
  JniEnvPtr &operator=(const JniEnvPtr &) = delete;
  static void JniThreadDestroyed(void *value);

private:
  JNIEnv *env_{nullptr};
};

} // namespace jni

#endif // __ANDROID__
