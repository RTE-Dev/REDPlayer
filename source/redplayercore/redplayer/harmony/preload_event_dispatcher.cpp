#include "preload_event_dispatcher.h"
#include "node_api_types.h"
#include "uv.h"
#include <string>

napi_env _preloadEventEnv;
napi_ref preloadEventListenerRef = nullptr;
napi_ref onPreloadEventFuncRef = nullptr;

struct CallbackContext {
  std::string instanceId;
  int32_t event;
  std::string url;
};

napi_value PreloadRegisterEventListener(napi_env env, napi_callback_info info) {
  _preloadEventEnv = env;
  size_t argc = 1;
  napi_value globalThisAdapter = nullptr;
  napi_get_cb_info(env, info, &argc, &globalThisAdapter, nullptr, nullptr);
  napi_value getEventListenerFunc = nullptr;
  napi_get_named_property(env, globalThisAdapter, "getListener",
                          &getEventListenerFunc);
  napi_value eventListener = nullptr;
  napi_call_function(env, globalThisAdapter, getEventListenerFunc, 0, nullptr,
                     &eventListener);
  napi_value onEventFunc = nullptr;
  napi_get_named_property(env, eventListener, "onNativeEvent", &onEventFunc);
  napi_create_reference(env, eventListener, 1, &preloadEventListenerRef);
  napi_create_reference(env, onEventFunc, 1, &onPreloadEventFuncRef);
  return nullptr;
}

void Preload_callOnEvent(napi_env env, std::string instanceId, int32_t event,
                         std::string url) {
  uv_loop_s *loop = nullptr;
  napi_get_uv_event_loop(_preloadEventEnv, &loop);

  uv_work_t *work = new uv_work_t;
  CallbackContext *context = new CallbackContext();
  context->instanceId = instanceId;
  context->event = event;
  context->url = url;
  work->data = context;

  uv_queue_work(
      loop, work,
      // work_cb
      [](uv_work_t *work) {},
      [](uv_work_t *work, int status) {
        CallbackContext *context =
            reinterpret_cast<CallbackContext *>(work->data);
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(_preloadEventEnv, &scope);
        if (scope == nullptr) {
          return;
        }
        const char *instanceIdC = context->instanceId.c_str();
        const char *urlC = context->url.c_str();
        size_t argc = 3;
        napi_value argv[3] = {nullptr};
        napi_create_string_utf8(_preloadEventEnv, instanceIdC,
                                strlen(instanceIdC) + 1, &argv[0]);
        napi_create_int32(_preloadEventEnv, context->event, &argv[1]);
        napi_create_string_utf8(_preloadEventEnv, urlC, strlen(urlC) + 1,
                                &argv[2]);
        napi_value eventListener = nullptr;
        napi_value onEventFunc = nullptr;
        napi_get_reference_value(_preloadEventEnv, preloadEventListenerRef,
                                 &eventListener);
        napi_get_reference_value(_preloadEventEnv, onPreloadEventFuncRef,
                                 &onEventFunc);
        napi_call_function(_preloadEventEnv, eventListener, onEventFunc, argc,
                           argv, nullptr);
        napi_close_handle_scope(_preloadEventEnv, scope);

        delete work;
        delete context;
      });
}
