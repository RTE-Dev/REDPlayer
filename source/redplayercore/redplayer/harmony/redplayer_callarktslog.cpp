#include "redplayer_callarktslog.h"
#include "hilog/log.h"
#include "uv.h"

napi_env _logEnv;
napi_ref logListenerRef = nullptr;
napi_ref onLogsFuncRef = nullptr;

struct RedLogCallbackContext {
  int level;
  std::string stringTag;
  std::string stringContent;
};

napi_value RegisterLogListener(napi_env env, napi_callback_info info) {
  _logEnv = env;
  size_t argc = 1;
  napi_value globalThisAdapter = nullptr;
  napi_get_cb_info(env, info, &argc, &globalThisAdapter, nullptr, nullptr);

  napi_value getLogListenerFunc = nullptr;
  napi_get_named_property(env, globalThisAdapter, "getLogsListener",
                          &getLogListenerFunc);

  napi_value logListener = nullptr;
  napi_call_function(env, globalThisAdapter, getLogListenerFunc, 0, nullptr,
                     &logListener);

  napi_value onLogsFunc = nullptr;
  napi_get_named_property(env, logListener, "onLogs", &onLogsFunc);

  napi_create_reference(env, logListener, 1, &logListenerRef);
  napi_create_reference(env, onLogsFunc, 1, &onLogsFuncRef);

  return nullptr;
}

void RedLog_callOnLogs(int level, std::string stringTag,
                       std::string stringContent) {
  uv_loop_s *loop = nullptr;
  napi_get_uv_event_loop(_logEnv, &loop);
  if (!loop) {
    return;
  }
  uv_work_t *work = new uv_work_t;
  RedLogCallbackContext *context = new RedLogCallbackContext();
  context->level = level;
  context->stringTag = stringTag;
  context->stringContent = stringContent;
  work->data = context;

  uv_queue_work(
      loop, work,
      // work_cb
      [](uv_work_t *work) {},
      [](uv_work_t *work, int status) {
        RedLogCallbackContext *context =
            reinterpret_cast<RedLogCallbackContext *>(work->data);
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(_logEnv, &scope);
        if (scope == nullptr) {
          return;
        }

        const char *tag = context->stringTag.c_str();
        const char *message = context->stringContent.c_str();
        size_t argc = 3;
        napi_value argv[3] = {nullptr};
        napi_create_int32(_logEnv, context->level, &argv[0]);
        napi_create_string_utf8(_logEnv, tag, strlen(tag) + 1, &argv[1]);
        napi_create_string_utf8(_logEnv, message, strlen(message) + 1,
                                &argv[2]);
        napi_value logListener = nullptr;
        napi_value onLogsFunc = nullptr;
        napi_get_reference_value(_logEnv, logListenerRef, &logListener);
        napi_get_reference_value(_logEnv, onLogsFuncRef, &onLogsFunc);
        napi_call_function(_logEnv, logListener, onLogsFunc, argc, argv,
                           nullptr);

        napi_close_handle_scope(_logEnv, scope);

        delete work;
        delete context;
      });
}
