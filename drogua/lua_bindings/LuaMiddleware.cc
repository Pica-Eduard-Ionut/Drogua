#include "LuaMiddleware.h"

#include "LuaRequest.h"
#include "LuaResponse.h"
#include "LuaAsyncContextRegistry.h"
#include "LuaRoutes.h"
#include "LuaAsyncContext.h"

#include <stdexcept>
#include <string>
#include <utility>

LuaMiddleware::LuaMiddleware(lua_State* L, const luabridge::LuaRef& function) : L_(L), function_(function) {
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

    // Push middleware function
    function_.push(L);
    // Push request
    auto requestResult = luabridge::Stack<LuaRequest*>::push(L, &req);
    if (!requestResult) {
        lua_settop(L, base);
        throw std::runtime_error("Failed to push LuaRequest: " + requestResult.message());
    }

    // Push response
    auto responseResult = luabridge::Stack<LuaResponse*>::push(L, &res);
    if (!responseResult) {
        lua_settop(L, base);
        throw std::runtime_error("Failed to push LuaResponse: " + responseResult.message());
    }

    // Context remains alive for the entire synchronous middleware execution
    ExecutionContext context{std::move(next)};
    /*
     * Create:
     *     next()
     * with context as an upvalue.
     */
    lua_pushlightuserdata(L, &context);
    lua_pushcclosure(L, &LuaMiddleware::luaNext, 1);

    // Call: middleware(req, res, next)
    const int status = lua_pcall(L, 3, 0, 0);

    if (status != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        lua_settop(L, base);
        throw std::runtime_error("Lua middleware failed: " + std::string(error ? error : "Unknown Lua error"));
    }

    lua_settop(L, base);
}

int LuaMiddleware::luaNext(lua_State* L) {
    auto* context = static_cast<ExecutionContext*>(lua_touserdata(L, lua_upvalueindex(1)));

    if (context == nullptr)
        return 0;

    if (context->next)
        context->next();

    return 0;
}

void LuaMiddleware::executeAsync(lua_State* L, LuaRequest& req, LuaResponse& res) {
    if (!L) {
        throw std::runtime_error("LuaMiddleware::executeAsync received null lua_State");
    }

    // Push middleware function
    function_.push(L);
    // Push request
    auto requestResult = luabridge::Stack<LuaRequest*>::push(L, &req);
    if (!requestResult) {
        throw std::runtime_error("Failed to push LuaRequest for async middleware: " + requestResult.message());
    }

    // Push response
    auto responseResult = luabridge::Stack<LuaResponse*>::push(L, &res);
    if (!responseResult) {
        throw std::runtime_error("Failed to push LuaResponse for async middleware: " + responseResult.message());
    }

    /*
     * Push async next()
     * luaNextAsync() retrieves the async context from LuaAsyncContextRegistry
     */
    lua_pushcfunction(L, &LuaMiddleware::luaNextAsync);
}

int LuaMiddleware::luaNextAsync(lua_State* L) {
    auto context = LuaAsyncContextRegistry::get(L);
    if (!context) {
        return luaL_error(L, "Async middleware next() has no async context");
    }

    if (!context->middlewareChain) {
        return luaL_error(L, "Async middleware next() has no middleware chain");
    }

    lua_pushboolean(L, 1);
    return lua_yieldk(L, 1, 0, &LuaMiddleware::nextAsyncContinuation);
}

int LuaMiddleware::nextAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    if (!L) return 0;

    auto context = LuaAsyncContextRegistry::get(L);
    if (!context) {
        return luaL_error(L, "Async middleware continuation has no async context");
    }

    if (!context->middlewareChain) {
        return luaL_error(L, "Async middleware continuation has no middleware chain");
    }

    if (status != LUA_YIELD) {
        return luaL_error(L, "Async middleware next() resumed with unexpected status");
    }

    ++context->middlewareIndex;
    // Continue through the middleware chain.
    if (context->middlewareIndex < context->middlewareChain->size()) {
        LuaMiddleware* middleware = (*context->middlewareChain)[context->middlewareIndex];
        if (!middleware) {
            return luaL_error(L, "Async middleware chain contains a null middleware");
        }

        middleware->executeAsync(L, *context->request, *context->response);

        /*
         * Actually invoke the downstream middleware.
         *
         * If it calls next(), the coroutine yields and the middleware chain continues through
         * nextAsyncContinuation(). When the downstream middleware chain eventually finishes,
         * downstreamContinuation() returns control to this middleware layer.
         */
        lua_callk(L, 3, 0, ctx, &LuaMiddleware::downstreamContinuation);

        return 0;
    }

    // No middleware remains -> execute the route handler.
    LuaCoroutineManager::pushFunction(context->coroutine, context->handler);
    auto requestResult = luabridge::Stack<LuaRequest*>::push(L, context->request.get());
    if (!requestResult) {
        return luaL_error(L, "Failed to push LuaRequest: %s", requestResult.message().c_str());
    }

    for (const auto& param : context->params) {
        lua_pushlstring(L, param.data(), param.size());
    }

    const int argumentCount = 1 + static_cast<int>(context->params.size());

    /*
     * Route returns one value.
     * If the route yields, routeContinuation() will handle the result when the coroutine resumes.
     */
    lua_callk(L, argumentCount, 1, ctx, &LuaMiddleware::routeContinuation);

    // Route returned synchronously.
    if (lua_gettop(L) < 1) {
        return luaL_error(L, "Async route handler did not return a result");
    }

    if (!captureRouteResult(L, *context)) {
        return luaL_error(L, "Async route handler must return a table or Drogua.Response");
    }

    return 0;
}

int LuaMiddleware::routeContinuation(lua_State* L, int status, lua_KContext ctx) {
    if (!L) return 0;

    auto context = LuaAsyncContextRegistry::get(L);
    if (!context) {
        return luaL_error(L, "Async route continuation has no async context");
    }

    if (status != LUA_YIELD) {
        return luaL_error(L, "Unexpected async route continuation status: %d", status);
    }

    if (lua_gettop(L) < 1) {
        return luaL_error(L, "Async route handler did not return a result");
    }

    /*
     * The yielded route has now returned its final value.
     * Use the same result handling as the synchronous route path in nextAsyncContinuation().
     */
    if (!captureRouteResult(L, *context)) {
        return luaL_error(L, "Async route handler must return a table or Drogua.Response");
    }

    return 0;
}

int LuaMiddleware::downstreamContinuation(lua_State* L, int status, lua_KContext ctx) {
    if (!L)
        return 0;

    auto context = LuaAsyncContextRegistry::get(L);
    if (!context) {
        return luaL_error(L, "Async middleware downstream continuation has no async context");
    }

    /*
     * The downstream middleware has finished.
     * We deliberately do not advance middlewareIndex here. nextAsyncContinuation() already
     * advanced it before entering the downstream middleware.
     *
     * Returning 0 resumes the middleware that originally called next(), giving us the onion-style unwind:
     *     MW1 -> MW2 -> MW3 -> route
     *        <- MW1  <- MW2  <- MW3
     */
    if (status != LUA_YIELD && status != LUA_OK) {
        return luaL_error(L, "Unexpected downstream middleware continuation status: %d", status);
    }

    return 0;
}

bool LuaMiddleware::captureRouteResult(lua_State* L, LuaAsyncContext& context) {
    auto result = luabridge::Stack<luabridge::LuaRef>::get(L, -1);

    if (!result)
        return false;

    auto luaResult = result.value();
    // Drogua.Response
    if (luaResult.isUserdata()) {
        auto response = luabridge::get<LuaResponse*>(L, -1);

        if (response) {
            context.response = std::make_unique<LuaResponse>(response.value()->response());
            context.hasRouteResponse = true;
            lua_pop(L, 1);
            return true;
        }
    }

    // Lua table -> JSON response
    if (luaResult.isTable()) {
        try {
            Json::Value json = LuaRoutes::luaTableToJson(luaResult);
            auto httpResponse = drogon::HttpResponse::newHttpJsonResponse(json);
            context.response = std::make_unique<LuaResponse>(httpResponse);
            context.hasRouteResponse = true;
            lua_pop(L, 1);
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
    }

    return false;
}
