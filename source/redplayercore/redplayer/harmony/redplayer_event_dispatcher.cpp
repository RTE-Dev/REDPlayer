#include "redplayer_event_dispatcher.h"
#include "napi/native_api.h"
#include "uv.h"
#include <thread>

napi_env _env;
napi_ref eventListenerRef = nullptr;
napi_ref onEventFuncRef = nullptr;

struct CallbackContext {
  std::string instanceId;
  int64_t time;
  int32_t event;
  int32_t arg1;
  int32_t arg2;
};

void repostInMainThread(std::string instanceId, int64_t time, int32_t event,
                        int32_t arg1, int32_t arg2) {
  uv_loop_s *loop = nullptr;
  napi_get_uv_event_loop(_env, &loop);

  uv_work_t *work = new uv_work_t;
  CallbackContext *context = new CallbackContext();
  context->instanceId = instanceId;
  context->time = time;
  context->event = event;
  context->arg1 = arg1;
  context->arg2 = arg2;
  work->data = context;

  uv_queue_work(
      loop, work,
      // work_cb
      [](uv_work_t *work) {},
      [](uv_work_t *work, int status) {
        CallbackContext *context =
            reinterpret_cast<CallbackContext *>(work->data);
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(_env, &scope);
        if (scope == nullptr) {
          return;
        }

        const char *instanceIdC = context->instanceId.c_str();
        size_t argc = 5;
        napi_value argv[5] = {nullptr};
        napi_create_string_utf8(_env, instanceIdC, strlen(instanceIdC) + 1,
                                &argv[0]);
        napi_create_int64(_env, context->time, &argv[1]);
        napi_create_int32(_env, context->event, &argv[2]);
        napi_create_int32(_env, context->arg1, &argv[3]);
        napi_create_int32(_env, context->arg2, &argv[4]);
        napi_value eventListener = nullptr;
        napi_value onEventFunc = nullptr;
        napi_get_reference_value(_env, eventListenerRef, &eventListener);
        napi_get_reference_value(_env, onEventFuncRef, &onEventFunc);
        napi_call_function(_env, eventListener, onEventFunc, argc, argv,
                           nullptr);
        napi_close_handle_scope(_env, scope);

        delete work;
        delete context;
      });
}

napi_value RegisterEventListener(napi_env env, napi_callback_info info) {
  _env = env;
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
  napi_create_reference(env, eventListener, 1, &eventListenerRef);
  napi_create_reference(env, onEventFunc, 1, &onEventFuncRef);
  return nullptr;
}

void RedPlayerCallbackOnEvent(std::string instanceId, int64_t time,
                              int32_t event, int32_t arg1, int32_t arg2,
                              napi_value params) {
  repostInMainThread(instanceId, time, event, arg1, arg2);
}
