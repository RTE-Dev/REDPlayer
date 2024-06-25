#pragma once

#include "js_native_api.h"
#include <string>

napi_value RegisterEventListener(napi_env env, napi_callback_info info);
napi_value UnRegisterEventListener(napi_env env, napi_callback_info info);

void RedPlayerCallbackOnEvent(std::string instanceId, int64_t time,
                              int32_t event, int32_t arg1, int32_t arg2,
                              napi_value params);
