#pragma once

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <functional>

class LuaRequest;
class LuaResponse;
class LuaMiddlewareManager;

class LuaMiddleware {
    friend class LuaMiddlewareManager;

public:
    using Next = std::function<void()>;

    void execute(LuaRequest& req, LuaResponse& res, Next next);

    const luabridge::LuaRef& function() const;

private:
    LuaMiddleware(lua_State* L, const luabridge::LuaRef& function);

    struct ExecutionContext {
        Next next;
    };

    static int luaNext(lua_State* L);

    lua_State* L_;
    luabridge::LuaRef function_;
};
