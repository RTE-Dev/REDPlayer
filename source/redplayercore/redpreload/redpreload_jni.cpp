#if defined(__ANDROID__)
#include <assert.h>
#include <inttypes.h>
#include <jni.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "RedLog.h"
#include "jni/ArrayList.h"
#include "jni/Bundle.h"
#include "jni/Debug.h"
#include "jni/Env.h"
#include "jni/Util.h"
#include "reddownload_datasource_wrapper.h"
#include "strategy/RedAdaptiveStrategy.h"

#define TAG "RedPreloadJNI"
#define JNI_CLASS_OPENREDPRELOAD_INTERFACE                                     \
  "com/xingin/openredpreload/OpenRedPreload"
static JavaVM *g_jvm = nullptr;
static jclass g_reddownload_class = nullptr;
static jmethodID g_onEvent_method = nullptr;

void speedcallback(void *opaque, int64_t size, int64_t speed,
                   int64_t cur_time) {
  redstrategycenter::RedStrategyCenter::GetInstance()->updateDownloadRate(
      size, speed, cur_time);
  if (!g_onEvent_method || !g_reddownload_class)
    return;

  std::unique_ptr<jni::JniEnvPtr> env_ptr = std::make_unique<jni::JniEnvPtr>();
  if (!env_ptr || !env_ptr->env()) {
    AV_LOGE(TAG, "%s: null env\n", __func__);
    return;
  }
  JNIEnv *env = env_ptr->env();

  jobject paramBundle = nullptr;
  do {
    paramBundle = JniAndroidOsBundleCatchAll(env);
    if (!paramBundle) {
      break;
    }
    JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "speed", speed);
    jobject weakObj = (jobject)opaque;
    env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                              0x30002 /* PRELOAD_EVENT_SPEED */, nullptr,
                              paramBundle);
    JniCheckExceptionCatchAll(env);
  } while (false);
  if (!paramBundle)
    JniDeleteLocalRefP(env, &paramBundle);
}

void AppEventCbWrapperImpl(void *opaque, int val, void *a1, void *a2) {
  if (!g_onEvent_method || !g_reddownload_class)
    return;

  std::unique_ptr<jni::JniEnvPtr> env_ptr = std::make_unique<jni::JniEnvPtr>();
  if (!env_ptr || !env_ptr->env()) {
    AV_LOGE(TAG, "%s: null env\n", __func__);
    return;
  }
  JNIEnv *env = env_ptr->env();

  switch (val) {
  case RED_EVENT_DID_FRAGMENT_COMPLETE_W: {
    if (!a1)
      return;
    jstring urlString = nullptr;
    jobject paramBundle = nullptr;
    do {
      paramBundle = JniAndroidOsBundleCatchAll(env);
      if (!paramBundle) {
        break;
      }
      RedIOTrafficWrapper *trafficEvent =
          reinterpret_cast<RedIOTrafficWrapper *>(a1);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "bytes",
                                       trafficEvent->bytes);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "cache_size",
                                       trafficEvent->cache_size);
      JniAndroidOsBundlePutStringCatchAll(env, paramBundle, "cache_file_path",
                                          trafficEvent->cache_path);
      urlString = JniNewStringUTFCatchAll(env, trafficEvent->url);
      if (!urlString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30001 /* PRELOAD_EVENT_IO_TRAFFIC */,
                                urlString, paramBundle);
      AV_LOGI(TAG, "RED_EVENT_DID_FRAGMENT_COMPLETE_W %s %d %d\n",
              trafficEvent->url, trafficEvent->bytes, trafficEvent->source);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (urlString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&urlString));
    if (paramBundle)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&paramBundle));
  } break;
  case RED_EVENT_DID_RELEASE_W: {
    do {
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x40000 /* PRELOAD_EVENT_RELEASE */, nullptr,
                                nullptr);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    JniDeleteGlobalRef(env, (jobject)opaque);
  } break;
  case RED_EVENT_WILL_DNS_PARSE_W: {
    if (!a1)
      return;
    jstring hostString = nullptr;
    do {
      hostString = JniNewStringUTFCatchAll(
          env, (reinterpret_cast<RedDnsInfoWrapper *>(a1))->domain);
      if (JniCheckExceptionCatchAll(env) || !hostString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30003 /* PRELOAD_EVENT_WILL_DNS_PARSE */,
                                hostString, nullptr);
      AV_LOGI(TAG, "RED_EVENT_WILL_DNS_PARSE_W %s",
              (reinterpret_cast<RedDnsInfoWrapper *>(a1))->domain);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (hostString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&hostString));
  } break;
  case RED_EVENT_DID_DNS_PARSE_W: {
    if (!a1)
      return;
    jobject paramBundle = nullptr;
    jstring hostString = nullptr;
    do {
      paramBundle = JniAndroidOsBundleCatchAll(env);
      if (!paramBundle) {
        break;
      }
      RedDnsInfoWrapper *dnsInfo = reinterpret_cast<RedDnsInfoWrapper *>(a1);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "status",
                                       dnsInfo->status);
      hostString = JniNewStringUTFCatchAll(env, dnsInfo->domain);
      if (JniCheckExceptionCatchAll(env) || !hostString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30004 /* PRELOAD_EVENT_DID_DNS_PARSE */,
                                hostString, paramBundle);
      AV_LOGI(TAG, "RED_EVENT_DID_DNS_PARSE_W %s %d\n", dnsInfo->domain,
              dnsInfo->status);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (paramBundle)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&paramBundle));
    if (hostString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&hostString));
  } break;
  case RED_EVENT_WILL_HTTP_OPEN_W: {
    do {
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30007 /* PRELOAD_EVENT_WILL_HTTP_OPEN */,
                                nullptr, nullptr);
      AV_LOGI(TAG, "RED_EVENT_WILL_HTTP_OPEN_W");
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
  } break;
  case RED_EVENT_DID_HTTP_OPEN_W: {
    if (!a1)
      return;
    jstring urlString = nullptr;
    jobject paramBundle = nullptr;
    do {
      paramBundle = JniAndroidOsBundleCatchAll(env);
      if (!paramBundle) {
        break;
      }
      RedHttpEventWrapper *httpEvent =
          reinterpret_cast<RedHttpEventWrapper *>(a1);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "error",
                                       httpEvent->error);
      JniAndroidOsBundlePutStringCatchAll(env, paramBundle, "wan_ip",
                                          httpEvent->wan_ip);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "http_rtt",
                                       httpEvent->http_rtt);
      urlString = JniNewStringUTFCatchAll(env, httpEvent->url);
      if (JniCheckExceptionCatchAll(env) || !urlString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30005 /* PRELOAD_EVENT_DID_HTTP_OPEN */,
                                urlString, paramBundle);
      AV_LOGI(TAG, "RED_EVENT_DID_HTTP_OPEN_W %s %s %d %d\n", httpEvent->url,
              httpEvent->wan_ip, httpEvent->http_rtt, httpEvent->source);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (urlString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&urlString));
    if (paramBundle)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&paramBundle));
  } break;
  case RED_CTRL_DID_TCP_OPEN_W: {
    if (!a1)
      return;
    jstring urlString = nullptr;
    jobject paramBundle = nullptr;
    do {
      paramBundle = JniAndroidOsBundleCatchAll(env);
      if (!paramBundle) {
        break;
      }
      RedTcpIOControlWrapper *tcpEvent =
          reinterpret_cast<RedTcpIOControlWrapper *>(a1);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "error",
                                       tcpEvent->error);
      JniAndroidOsBundlePutStringCatchAll(env, paramBundle, "dst_ip",
                                          tcpEvent->ip);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "tcp_rtt",
                                       tcpEvent->tcp_rtt);
      urlString = JniNewStringUTFCatchAll(env, tcpEvent->url);
      if (JniCheckExceptionCatchAll(env) || !urlString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30006 /* PRELOAD_EVENT_DID_TCP_OPEN */,
                                urlString, paramBundle);
      AV_LOGI(TAG, "RED_CTRL_DID_TCP_OPEN_W %s %s %d %d\n", tcpEvent->url,
              tcpEvent->ip, tcpEvent->tcp_rtt, tcpEvent->source);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (urlString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&urlString));
    if (paramBundle)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&paramBundle));
  } break;
  case RED_EVENT_ERROR_W: {
    if (!a1)
      return;
    jstring urlString = nullptr;
    jobject paramBundle = nullptr;
    do {
      paramBundle = JniAndroidOsBundleCatchAll(env);
      if (!paramBundle) {
        break;
      }
      RedErrorEventWrapper *errorEvent =
          reinterpret_cast<RedErrorEventWrapper *>(a1);
      JniAndroidOsBundlePutIntCatchAll(env, paramBundle, "error",
                                       errorEvent->error);
      urlString = JniNewStringUTFCatchAll(env, errorEvent->url);
      if (JniCheckExceptionCatchAll(env) || !urlString)
        break;
      jobject weakObj = (jobject)opaque;
      env->CallStaticVoidMethod(g_reddownload_class, g_onEvent_method, weakObj,
                                0x30008 /* PRELOAD_EVENT_ERROR */, urlString,
                                paramBundle);
      AV_LOGI(TAG, "RED_EVENT_ERROR_W %s %d %d\n", errorEvent->url,
              errorEvent->error, errorEvent->source);
      if (JniCheckExceptionCatchAll(env))
        break;
    } while (false);
    if (urlString)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&urlString));
    if (paramBundle)
      JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&paramBundle));
  } break;
  default:
    break;
  }
}

static jstring Reddownload_open(JNIEnv *env, jclass clazz, jstring jurl,
                                jstring jpath, jlong jdownsize, jobject obj,
                                jobject jbundle) {
  if (jurl == nullptr || jpath == nullptr)
    return nullptr;

  // DownLoadCb
  DownLoadCbWrapper wrapper = {0};
  wrapper.appctx = JniNewGlobalRefCatchAll(
      env,
      obj); // need to release when preload finished(RED_EVENT_DID_RELEASE_W)
  if (!wrapper.appctx) {
    return nullptr;
  }
  wrapper.appcb = AppEventCbWrapperImpl;
  wrapper.speedcb = speedcallback;

  // DownLoadOpt
  std::string path = JniGetStringUTFCharsCatchAll(env, jpath);
  jstring jreferer =
      JniAndroidOsBundleGetStringCatchAll(env, jbundle, "referer");
  std::string referer = "";
  if (jreferer) {
    referer = JniGetStringUTFCharsCatchAll(env, jreferer);
  }
  jstring juser_agent =
      JniAndroidOsBundleGetStringCatchAll(env, jbundle, "user_agent");
  std::string user_agent = "";
  if (juser_agent) {
    user_agent = JniGetStringUTFCharsCatchAll(env, juser_agent);
  }
  jstring jheader = JniAndroidOsBundleGetStringCatchAll(env, jbundle, "header");
  std::string header = "";
  if (jheader) {
    header = JniGetStringUTFCharsCatchAll(env, jheader);
  }
  jint downloadtype =
      JniAndroidOsBundleGetIntCatchAll(env, jbundle, "download_type", 0);
  jlong capacity = JniAndroidOsBundleGetLongCatchAll(env, jbundle, "capacity");
  jlong cache_max_entries =
      JniAndroidOsBundleGetLongCatchAll(env, jbundle, "cache_max_entries");

  jint use_https =
      JniAndroidOsBundleGetIntCatchAll(env, jbundle, "use_https", 0);
  jint is_json = JniAndroidOsBundleGetIntCatchAll(env, jbundle, "is_json", 0);
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  if (downloadtype > 0) {
    opt.DownLoadType = downloadtype;
  }
  if (capacity > 0) {
    opt.cache_max_dir_capacity = capacity;
  }
  if (cache_max_entries > 0) {
    opt.cache_max_entries = cache_max_entries;
  }
  opt.cache_file_dir = path.c_str();
  opt.PreDownLoadSize = jdownsize;
  if (!referer.empty()) {
    opt.referer = referer.c_str();
  }
  if (!user_agent.empty()) {
    opt.useragent = user_agent.c_str();
  }
  if (!header.empty()) {
    opt.headers = header.c_str();
  }
  opt.use_https = use_https;

  // Strategy
  std::string url = JniGetStringUTFCharsCatchAll(env, jurl);
  if (is_json) {
    std::unique_ptr<redstrategycenter::strategy::RedAdaptiveStrategy> strategy =
        nullptr;
    strategy =
        std::make_unique<redstrategycenter::strategy::RedAdaptiveStrategy>(
            AbstractAdaptationLogic::LogicType::Adaptive);
    if (strategy) {
      strategy->setPlaylist(url);
      strategy->setReddownloadUrlCachedFunc(
          reddownload_datasource_wrapper_cache_size);
      url = strategy->getInitialUrl(strategy->getInitialRepresentation());
    }
  }

  // Open
  AV_LOGI(TAG, "preload open %s %" PRId64 ".\n", url.c_str(), jdownsize);
  reddownload_datasource_wrapper_open(url.c_str(), &wrapper, &opt);

  jstring jpreload_url = JniNewStringUTFCatchAll(env, url);
  return jpreload_url;
}

static void Reddownload_release(JNIEnv *env, jclass clazz, jstring jurl) {
  std::string url = JniGetStringUTFCharsCatchAll(env, jurl);
  AV_LOGI(TAG, "preload release %s\n", url.c_str());
  reddownload_datasource_wrapper_close(url.c_str(), 0);
}

static void Reddownload_stop(JNIEnv *env, jclass clazz) {
  AV_LOGI(TAG, "preload stop begin\n");
  reddownload_datasource_wrapper_stop(nullptr, 0);
  AV_LOGI(TAG, "preload stop end\n");
}

static jstring Reddownload_getCacheFilePath(JNIEnv *env, jclass clazz,
                                            jstring jpath, jstring jurl) {
  if (jpath == nullptr || jurl == nullptr)
    return nullptr;
  jstring jcache_file_path = nullptr;
  std::string path = JniGetStringUTFCharsCatchAll(env, jpath);
  std::string url = JniGetStringUTFCharsCatchAll(env, jurl);

  char *cache_file_path = nullptr;
  int ret = reddownload_get_cache_file_path(path.c_str(), url.c_str(),
                                            &cache_file_path);
  if (!ret) {
    jcache_file_path = JniNewStringUTFCatchAll(env, cache_file_path);
  }
  AV_LOGI(TAG, "%s: path %s\n", __func__, cache_file_path);
  if (cache_file_path) {
    free(cache_file_path);
    cache_file_path = nullptr;
  }
  return jcache_file_path;
}

static int Reddownload_deleteCache(JNIEnv *env, jclass clazz, jstring jpath,
                                   jstring juri, int is_full_url) {
  if (jpath == nullptr || juri == nullptr)
    return 0;
  std::string uri = JniGetStringUTFCharsCatchAll(env, juri);
  std::string path = JniGetStringUTFCharsCatchAll(env, jpath);
  AV_LOGI(TAG, "%s %s %s\n", __func__, uri.c_str(), path.c_str());
  reddownload_delete_cache(path.c_str(), uri.c_str(), is_full_url);
  return 0;
}

static jobjectArray Reddownload_getAllCachedFile(JNIEnv *env, jclass clazz,
                                                 jstring jpath) {
  if (jpath == nullptr)
    return nullptr;
  std::string path = JniGetStringUTFCharsCatchAll(env, jpath);
  AV_LOGI(TAG, "%s path %s\n", __func__, path.c_str());

  char **cached_file;
  int cached_file_num = 0;
  reddownload_get_all_cached_file(path.c_str(), &cached_file, &cached_file_num);
  if (cached_file_num <= 0) {
    return nullptr;
  }
  jobjectArray jcached_file = (jobjectArray)env->NewObjectArray(
      cached_file_num, env->FindClass("java/lang/String"),
      JniNewStringUTFCatchAll(env, ""));
  for (int i = 0; i < cached_file_num; i++) {
    env->SetObjectArrayElement(
        jcached_file, i, JniNewStringUTFCatchAll(env, *(cached_file + i)));
    free(*(cached_file + i));
  }
  return jcached_file;
}

static int64_t Reddownload_getCacheSize(JNIEnv *env, jclass clazz,
                                        jstring jpath, jstring juri,
                                        int is_full_url) {
  if (jpath == nullptr || juri == nullptr)
    return 0;
  std::string uri = JniGetStringUTFCharsCatchAll(env, juri);
  std::string path = JniGetStringUTFCharsCatchAll(env, jpath);
  int64_t cache_size = reddownload_datasource_wrapper_cache_size_by_uri(
      path.c_str(), uri.c_str(), is_full_url);
  return cache_size;
}

static JNINativeMethod g_methods[] = {
    {"_open",
     "(Ljava/lang/String;Ljava/lang/String;JLjava/lang/Object;Landroid/os/"
     "Bundle;)Ljava/lang/String;",
     reinterpret_cast<void *>(Reddownload_open)},
    {"_release", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(Reddownload_release)},
    {"_stop", "()V", reinterpret_cast<void *>(Reddownload_stop)},
    {"_deleteCache", "(Ljava/lang/String;Ljava/lang/String;I)I",
     reinterpret_cast<void *>(Reddownload_deleteCache)},
    {"_getAllCachedFile", "(Ljava/lang/String;)[Ljava/lang/String;",
     reinterpret_cast<void *>(Reddownload_getAllCachedFile)},
    {"_getCacheSize", "(Ljava/lang/String;Ljava/lang/String;I)J",
     reinterpret_cast<void *>(Reddownload_getCacheSize)},
    {"_getCacheFilePath",
     "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
     reinterpret_cast<void *>(Reddownload_getCacheFilePath)}};
int numMethods = sizeof(g_methods) / sizeof(g_methods[0]);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  JNIEnv *env = nullptr;

  g_jvm = vm;
  if (!vm ||
      vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  g_reddownload_class =
      JniGetClassGlobalRefCatchAll(env, JNI_CLASS_OPENREDPRELOAD_INTERFACE);
  g_onEvent_method = JniGetStaticClassMethodCatchAll(
      env, g_reddownload_class, "onEvent",
      "(Ljava/lang/Object;ILjava/lang/String;Landroid/os/Bundle;)V");

  env->RegisterNatives(g_reddownload_class, g_methods,
                       sizeof(g_methods) / sizeof((g_methods)[0]));

  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved) {
  JNIEnv *env = nullptr;
  if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return;
  }

  JniDeleteGlobalRef(env, g_reddownload_class);
}

#endif
