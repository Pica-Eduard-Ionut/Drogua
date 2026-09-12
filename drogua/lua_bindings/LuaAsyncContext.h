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

struct LuaAsyncContext {
    LuaCoroutineManager::Ptr coroutine;
    std::unique_ptr<LuaRequest> request;
    std::function<void(const drogon::HttpResponsePtr&)> callback;

    std::function<void()> resume;
    // async route handler
    luabridge::LuaRef handler;
    std::vector<std::string> params;

    LuaAsyncContext(lua_State* L)
        : handler(L) {
    }
};

struct LuaAsyncDatabaseContext {
    LuaCoroutineManager::Ptr coroutine;
    std::function<void()> resume;
    std::function<void(const drogon::HttpResponsePtr&)> callback;

    std::shared_ptr<LuaResult> asyncResult;
    std::string asyncError;
};

struct LuaAsyncTransactionContext {
    LuaCoroutineManager::Ptr coroutine;
    std::function<void()> resume;
    std::function<void(const drogon::HttpResponsePtr&)> callback;

    std::shared_ptr<LuaResult> asyncResult;
    std::string asyncError;

    std::shared_ptr<LuaTransaction> transaction;
};

struct LuaAsyncMiddlewareContext {
    LuaCoroutineManager::Ptr coroutine;
    std::function<void()> resume;
    std::function<void(const drogon::HttpResponsePtr&)> callback;

    std::unique_ptr<LuaRequest> request;
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