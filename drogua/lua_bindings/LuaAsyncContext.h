#pragma once

#include <drogon/drogon.h>
#include <lua.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "LuaCoroutineManager.h"
#include "LuaRequest.h"
#include "LuaResult.h"

struct LuaAsyncContext {
    LuaCoroutineManager::Ptr coroutine;

    std::unique_ptr<LuaRequest> request;
    drogon::HttpRequestPtr httpRequest;

    std::vector<std::string> params;

    std::function<void(const drogon::HttpResponsePtr&)> callback;

    // Result/error from an asynchronous operation
    std::shared_ptr<LuaResult> asyncResult;
    std::string asyncError;

    std::function<void()> resume;
};