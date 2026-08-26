#include "LuaMiddleware.h"

#include "LuaRequest.h"
#include "LuaResponse.h"

#include <stdexcept>
#include <string>
#include <utility>

LuaMiddleware::LuaMiddleware(lua_State* L, const luabridge::LuaRef& function)
    : L_(L), function_(function) {
    if (L_ == nullptr)
        throw std::runtime_error("LuaMiddleware received a null lua_State");

    if (!function_.isFunction())
        throw std::runtime_error("LuaMiddleware requires a Lua function");
}

const luabridge::LuaRef& LuaMiddleware::function() const {
    return function_;
}

void LuaMiddleware::execute(LuaRequest& req, LuaResponse& res, Next next) {
    lua_State* L = L_;
    const int base = lua_gettop(L);

    /*
     * Push middleware function.
     */
    function_.push(L);

    /*
     * Push request.
     */
    auto requestResult = luabridge::Stack<LuaRequest*>::push(L, &req);

    if (!requestResult) {
        lua_settop(L, base);

        throw std::runtime_error(
            "Failed to push LuaRequest: " + requestResult.message());
    }

    /*
     * Push response.
     */
    auto responseResult = luabridge::Stack<LuaResponse*>::push(L, &res);

    if (!responseResult) {
        lua_settop(L, base);

        throw std::runtime_error(
            "Failed to push LuaResponse: " + responseResult.message());
    }

    /*
     * Context remains alive for the entire synchronous
     * middleware execution.
     */
    ExecutionContext context{std::move(next)};

    /*
     * Create:
     *
     *     next()
     *
     * with context as an upvalue.
     */
    lua_pushlightuserdata(L, &context);
    lua_pushcclosure(L, &LuaMiddleware::luaNext, 1);

    /*
     * Call:
     *
     *     middleware(req, res, next)
     */
    const int status = lua_pcall(L, 3, 0, 0);

    if (status != LUA_OK) {
        const char* error = lua_tostring(L, -1);

        lua_settop(L, base);

        throw std::runtime_error(
            "Lua middleware failed: " +
            std::string(error ? error : "Unknown Lua error"));
    }

    lua_settop(L, base);
}

int LuaMiddleware::luaNext(lua_State* L) {
    auto* context = static_cast<ExecutionContext*>(
        lua_touserdata(L, lua_upvalueindex(1)));

    if (context == nullptr)
        return 0;

    if (context->next)
        context->next();

    return 0;
}
