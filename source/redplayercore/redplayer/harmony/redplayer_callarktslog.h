#pragma once

#include "js_native_api.h"
#include "napi/native_api.h"
#include <string>

napi_value RegisterLogListener(napi_env env, napi_callback_info info);

void RedLog_callOnLogs(int level, std::string stringTag,
                       std::string stringContent);
