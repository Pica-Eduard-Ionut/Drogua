#pragma once

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <functional>

class LuaRequest;
class LuaResponse;
class LuaMiddlewareManager;
struct LuaAsyncContext;

class LuaMiddleware {
    friend class LuaMiddlewareManager;

public:
    using Next = std::function<void()>;

    void execute(LuaRequest& req, LuaResponse& res, Next next);

    void executeAsync(lua_State* L, LuaRequest& req, LuaResponse& res);

    const luabridge::LuaRef& function() const;

private:
    LuaMiddleware(lua_State* L, const luabridge::LuaRef& function);

    struct ExecutionContext {
        Next next;
    };

    static int luaNext(lua_State* L);
    //async
    static int luaNextAsync(lua_State* L);
    static int nextAsyncContinuation(lua_State* L, int status, lua_KContext ctx);
    static int downstreamContinuation(lua_State* L, int status, lua_KContext ctx);
    static int routeContinuation(lua_State* L, int status, lua_KContext ctx);
    static bool captureRouteResult(lua_State* L, LuaAsyncContext& context);

    lua_State* L_;
    luabridge::LuaRef function_;
};
