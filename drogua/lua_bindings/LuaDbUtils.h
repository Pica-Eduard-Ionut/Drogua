// LuaDbUtils.h
#pragma once

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <drogon/orm/DbClient.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>

struct LuaAsyncRouteContext;

namespace LuaDbUtils {
    static inline void bindLuaParameters(drogon::orm::internal::SqlBinder& binder, const luabridge::LuaRef& params) {
        if (!params.isTable()) 
            throw std::runtime_error("Database query parameters must be a Lua table");

        const auto length = params.length();
        for (int i = 1; i <= length; ++i) {
            luabridge::LuaRef value(params[i]);
            if (value.isNil()) binder << nullptr;

            else if (value.isBool()) binder << value.cast<bool>().value();

            else if (value.isNumber()) {
                lua_State* L = value.state();
                value.push(L);
                if (lua_isinteger(L, -1)) {
                    lua_Integer integerValue = lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    binder << static_cast<int64_t>(integerValue);

                } else {
                    lua_Number numberValue = lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    binder << static_cast<double>(numberValue);
                }
            } 
            else if (value.isString()) binder << value.cast<std::string>().value();

            else throw std::runtime_error("Unsupported database parameter at index " + std::to_string(i));
        }
    }
    
    template<typename TargetT>
    std::pair<TargetT*, std::shared_ptr<LuaAsyncRouteContext>> extractAsyncLuaArgs(lua_State* L, const char* funcName) {
        if (!L || !lua_isuserdata(L, 1)) luaL_error(L, "%s expected a %s", funcName, typeid(TargetT).name());

        auto result = luabridge::get<TargetT*>(L, 1);
        if (!result) 
            luaL_error(L, "Invalid %s: %s", typeid(TargetT).name(), result.message().c_str());
        if (!result.value()) 
            luaL_error(L, "%s is null", typeid(TargetT).name());

        auto routeContext = LuaAsyncContextRegistry::get<LuaAsyncRouteContext>(L);
        if (!routeContext || !routeContext->coroutine) 
            luaL_error(L, "%s must be called from an async route with active coroutine", funcName);

        return {result.value(), routeContext};
    }

    template<typename ContextT>
    int handleQueryContinuation(lua_State* L, int status, lua_KContext ctx, const char* operationName) {
        (void)ctx;
        if (!L) return 0;

        auto context = LuaAsyncContextRegistry::get<ContextT>(L);
        if (!context) 
            return luaL_error(L, "%s continuation has no async context", operationName);

        if (status != LUA_YIELD) {
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "%s continuation resumed with unexpected status", operationName);
        }

        if (!context->asyncError.empty()) {
            std::string error = std::move(context->asyncError);
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "Async %s failed: %s", operationName, error.c_str());
        }

        if (!context->asyncResult) {
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "Async %s completed without a result", operationName);
        }

        auto result = std::move(context->asyncResult);
        LuaAsyncContextRegistry::clear<ContextT>(L);

        auto pushResult = luabridge::Stack<std::shared_ptr<LuaResult>>::push(L, result);
        if (!pushResult) 
            return luaL_error(L, "Failed to push async %s result: %s", operationName, pushResult.message().c_str());

        return 1;
    }

    template<typename ContextT>
    int handleBeginTransactionContinuation(lua_State* L, int status, lua_KContext ctx, const char* operationName) {
        (void)ctx;
        if (!L) return 0;

        auto context = LuaAsyncContextRegistry::get<ContextT>(L);
        if (!context) 
            return luaL_error(L, "%s continuation has no async context", operationName);

        if (status != LUA_YIELD) {
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "%s continuation resumed with unexpected status", operationName);
        }

        if (!context->asyncError.empty()) {
            std::string error = std::move(context->asyncError);
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "Async %s failed: %s", operationName, error.c_str());
        }

        if (!context->transaction) {
            LuaAsyncContextRegistry::clear<ContextT>(L);
            return luaL_error(L, "Async %s completed without a transaction", operationName);
        }

        auto transaction = std::move(context->transaction);
        LuaAsyncContextRegistry::clear<ContextT>(L);

        auto pushResult = luabridge::Stack<std::shared_ptr<LuaTransaction>>::push(L, transaction);
        if (!pushResult) 
            return luaL_error(L, "Failed to push async %s result: %s", operationName, pushResult.message().c_str());

        return 1;
    }

    template<typename ContextT>
    static inline void clearAsyncContext(lua_State* L) {
        if (L) LuaAsyncContextRegistry::clear<ContextT>(L);
    }
} // namespace LuaDbUtils
