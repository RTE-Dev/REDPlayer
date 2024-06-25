#pragma once

#include "js_native_api.h"
#include "napi/native_api.h"
#include <string>

struct PreloadObject {
  std::string instanceId;
};
napi_value PreloadRegisterEventListener(napi_env env, napi_callback_info info);
void Preload_callOnEvent(napi_env env, std::string instanceId, int32_t event,
                         std::string url);
