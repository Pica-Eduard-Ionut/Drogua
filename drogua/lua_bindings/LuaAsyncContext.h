#pragma once

#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "LuaCoroutineManager.h"
#include "LuaRequest.h"
#include "LuaResult.h"
#include "LuaTransaction.h"
#include "LuaMiddlewareManager.h"
#include "LuaResponse.h"
#include "LuaHttpResult.h"

struct LuaAsyncContext {
    LuaCoroutineManager::Ptr coroutine;
    std::function<void()> resume;
    trantor::EventLoop* ownerLoop = nullptr;
    std::function<void(const drogon::HttpResponsePtr&)> callback;
    // Shared pointer allows Route, Database, and Transaction contexts to safely check connection status
    std::shared_ptr<LuaRequest> request; 
};

struct LuaAsyncRouteContext : LuaAsyncContext {
    // std::unique_ptr<LuaRequest> request;
    luabridge::LuaRef handler;
    std::vector<std::string> params;

    LuaAsyncRouteContext(lua_State* L)
        : handler(L) {
    }
};

struct LuaAsyncDatabaseContext : LuaAsyncContext {
    std::shared_ptr<LuaResult> asyncResult;
    std::string asyncError;
};

struct LuaAsyncTransactionContext : LuaAsyncContext {
    std::shared_ptr<LuaResult> asyncResult;
    std::string asyncError;

    std::shared_ptr<LuaTransaction> transaction;
};

struct LuaAsyncMiddlewareContext : LuaAsyncContext {
    // std::unique_ptr<LuaRequest> request;
    std::unique_ptr<LuaResponse> response;
    bool hasRouteResponse = false;

    std::size_t middlewareIndex = 0;
    const LuaMiddlewareManager::MiddlewareChain* middlewareChain = nullptr;

    luabridge::LuaRef handler;
    std::vector<std::string> params;

    LuaAsyncMiddlewareContext(lua_State* L)
        : handler(L) {
    }
};

struct LuaAsyncHttpContext : LuaAsyncContext {
    std::shared_ptr<LuaHttpResult> asyncResult;
    std::string asyncError;
    drogon::HttpRequestPtr httpRequest;
    
    int retryCount = 0;
    int maxRetries = 0;
    int retryDelayMs = 100;
    
    std::string url;
};