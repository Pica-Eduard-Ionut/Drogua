#include "LuaRoutes.h"
#include "LuaRequest.h"
#include "LuaResponse.h"

#include <stdexcept>
#include <string>

using namespace drogon;

int LuaRoutes::luaGet(lua_State *L) {
    return luaRegister(L, Get, "get");
}

int LuaRoutes::luaPost(lua_State *L) {
    return luaRegister(L, Post, "post");
}

int LuaRoutes::luaPut(lua_State *L) {
    return luaRegister(L, Put, "put");
}

int LuaRoutes::luaDelete(lua_State *L) {
    return luaRegister(L, Delete, "delete");
}

int LuaRoutes::luaPatch(lua_State *L) {
    return luaRegister(L, Patch, "patch");
}

// Async Routes
int LuaRoutes::luaGetAsync(lua_State *L) {
    return luaRegisterAsync(L, Get, "getAsync");
}

int LuaRoutes::luaPostAsync(lua_State *L) {
    return luaRegisterAsync(L, Post, "postAsync");
}

int LuaRoutes::luaPutAsync(lua_State *L) {
    return luaRegisterAsync(L, Put, "putAsync");
}

int LuaRoutes::luaDeleteAsync(lua_State *L) {
    return luaRegisterAsync(L, Delete, "deleteAsync");
}

int LuaRoutes::luaPatchAsync(lua_State *L) {
    return luaRegisterAsync(L, Patch, "patchAsync");
}

int LuaRoutes::luaRegister(lua_State *L, drogon::HttpMethod method, const char *methodName) {
    try {
        const int argc = lua_gettop(L);
        auto args = parseRouteArgs(L, methodName);

        registerMiddleware(L, argc, method, args.path);
        registerRoute(args.path, method, args.handler);

    } catch (const std::exception &e) {
        return luaL_error(L, "%s", e.what());
    }

    return 0;
}

int LuaRoutes::luaRegisterAsync(lua_State *L, drogon::HttpMethod method, const char *methodName) {
    try {
        const int argc = lua_gettop(L);
        auto args = parseRouteArgs(L, methodName);

        registerMiddleware(L, argc, method, args.path);
        registerAsyncRoute(args.path, method, args.handler);

    } catch (const std::exception &e) {
        return luaL_error(L, "%s", e.what());
    }

    return 0;
}


Json::Value LuaRoutes::luaValueToJson(lua_State *L, int index) {
    switch (lua_type(L, index)) {
        case LUA_TSTRING:
            return Json::Value(lua_tostring(L, index));

        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
                return Json::Value(static_cast<Json::Int64>(lua_tointeger(L, index)));

            return Json::Value(lua_tonumber(L, index));

        case LUA_TBOOLEAN:
            return Json::Value(lua_toboolean(L, index) != 0);

        case LUA_TNIL:
            return Json::nullValue;

        case LUA_TTABLE:
        {
            luabridge::LuaRef nested = luabridge::LuaRef::fromStack(L, index);

            return luaTableToJson(nested);
        }

        default:
            return Json::nullValue;
    }
}

Json::Value LuaRoutes::luaTableToJson(const luabridge::LuaRef &table) {
    if (!table.isTable())
        return Json::nullValue;

    lua_State *L = table.state();

    table.push(L);
    const int tableIndex = lua_gettop(L);

    // Determine whether this is an array.
    bool isArray = true;
    lua_Integer maxIndex = 0;
    lua_Integer count = 0;

    lua_pushnil(L);

    while (lua_next(L, tableIndex) != 0) {
        if (lua_type(L, -2) == LUA_TNUMBER && lua_isinteger(L, -2)) {
            const lua_Integer index = lua_tointeger(L, -2);
            if (index > 0) {
                ++count;
                if (index > maxIndex)
                    maxIndex = index;
            }
            else {
                isArray = false;
            }
        }
        else {
            isArray = false;
        }

        lua_pop(L, 1);
    }

    // A Lua table is an array only if its integer keys are exactly 1..N with no holes.
    if (isArray && maxIndex != count)
        isArray = false;

    Json::Value result = isArray ? Json::Value(Json::arrayValue) : Json::Value(Json::objectValue);
    // Iterate through the table again and convert its values.
    lua_pushnil(L);

    while (lua_next(L, tableIndex) != 0) {
        const int keyType = lua_type(L, -2);

        // number indexed array
        if (isArray && keyType == LUA_TNUMBER && lua_isinteger(L, -2)) {
            const lua_Integer index = lua_tointeger(L, -2);
            if (index > 0) {
                result[static_cast<Json::ArrayIndex>(index - 1)] = luaValueToJson(L, -1);
            }
        }

        // string indexed array
        else if (!isArray && keyType == LUA_TSTRING) {
            const char *key = lua_tostring(L, -2);
            result[key] = luaValueToJson(L, -1);
        }

        lua_pop(L, 1);
    }
    // Remove the table.
    lua_pop(L, 1);

    return result;
}

void LuaRoutes::registerRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    if (!handler.isTable() && !handler.isFunction())
        throw std::runtime_error("Route requires a Lua table or function");

    switch (countPathParameters(path)) {
        case 0:
            registerRouteImpl<>(path, method, handler);
            break;

        case 1:
            registerRouteImpl<std::string>(path, method, handler);
            break;

        case 2:
            registerRouteImpl<std::string, std::string>(path, method, handler);
            break;

        case 3:
            registerRouteImpl<std::string, std::string, std::string>(path, method, handler);
            break;

        case 4:
            registerRouteImpl<std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        case 5:
            registerRouteImpl<std::string, std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        case 6:
            registerRouteImpl<std::string, std::string, std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        default:
            throw std::runtime_error("Routes may have at most 6 path parameters");
    }
}


drogon::HttpResponsePtr LuaRoutes::executeHandler(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params) {
    if (handler.isTable())
        return executeLuaTable(handler);

    if (handler.isFunction())
        return executeLuaFunction(handler, req, params);

    throw std::runtime_error("Invalid Lua route handler");
}

drogon::HttpResponsePtr LuaRoutes::executeLuaTable(const luabridge::LuaRef &handler) {
    Json::Value json = luaTableToJson(handler);
    return drogon::HttpResponse::newHttpJsonResponse(json);
}

drogon::HttpResponsePtr LuaRoutes::executeLuaFunction(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params) {
    lua_State *L = handler.state();
    const int base = lua_gettop(L);
    LuaRequest luaRequest(req);
    handler.push(L);

    auto pushResult = luabridge::Stack<LuaRequest *>::push(L, &luaRequest);
    if (!pushResult) {
        lua_settop(L, base);
        throw std::runtime_error("Failed to push LuaRequest: " + pushResult.message());
    }

    for (const auto &param : params)
        lua_pushlstring(L, param.data(), param.size());

    const int status = lua_pcall(L, 1 + static_cast<int>(params.size()), 1, 0);
    if (status != LUA_OK) {
        const char *error = lua_tostring(L, -1);
        lua_settop(L, base);
        throw std::runtime_error("Lua route handler failed: " + std::string(error ? error : "Unknown Lua error"));
    }

    auto result = luabridge::Stack<luabridge::LuaRef>::get(L, -1);
    if (!result) {
        lua_settop(L, base);
        throw std::runtime_error("Failed to retrieve Lua route result: " + result.message());
    }

    auto luaResult = result.value();

    // LuaResponse
    if (luaResult.isUserdata()) {
        // auto response = luabridge::get<LuaResponse>(L, -1);
        auto response = luabridge::get<LuaResponse *>(L, -1);

        if (response)
        {
            auto httpResponse = response.value()->response();
            lua_settop(L, base);
            return httpResponse;
        }
    }

    // Lua table
    if (luaResult.isTable()) {
        Json::Value json = luaTableToJson(luaResult);
        lua_settop(L, base);

        return drogon::HttpResponse::newHttpJsonResponse(json);
    }

    lua_settop(L, base);
    throw std::runtime_error("Lua route handler must return a table or Drogua.Response");
}

void LuaRoutes::sendJsonResponse(const Json::Value &json, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(json);
    callback(response);
}

void LuaRoutes::sendErrorResponse(const std::string &message, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Json::Value error;
    error["error"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(error);
    response->setStatusCode(drogon::k500InternalServerError);
    callback(response);
}

size_t LuaRoutes::countPathParameters(const std::string &path) {
    size_t count = 0;
    bool inside = false;

    for (char c : path) {
        if (c == '{') {
            if (inside)
                throw std::runtime_error("Invalid route: nested '{'");
            inside = true;
            ++count;
        }
        else if (c == '}') {
            if (!inside)
                throw std::runtime_error("Invalid route: unexpected '}'");
            inside = false;
        }
    }

    if (inside)
        throw std::runtime_error("Invalid route: missing '}'");

    return count;
}

drogon::HttpResponsePtr LuaRoutes::executeMiddlewareChain(const LuaMiddlewareManager::MiddlewareChain &chain, std::size_t index, LuaRequest &req, LuaResponse &res, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &httpReq, const std::vector<std::string> &params) {
    /*
     * No more middleware.
     * Execute the actual route handler.
     */
    if (index >= chain.size())
        return executeHandler(handler, httpReq, params);

    LuaMiddleware *middleware = chain[index];

    if (middleware == nullptr)
        throw std::runtime_error("Middleware chain contains a null middleware");

    /*
     * When this middleware calls next(),
     * recursively execute the next middleware.
     */
    bool nextCalled = false;
    drogon::HttpResponsePtr response;

    middleware->execute(req, res, [&]() {
        nextCalled = true;
        response = executeMiddlewareChain(chain, index + 1, req, res, handler, httpReq, params); 
    });

    /*
     * Middleware called next(), so return
     * the response from the remaining chain.
     */
    if (nextCalled)
        return response;

    /*
     * Middleware did not call next(), so it
     * terminated the chain.
     */
    return res.response();
}

drogon::HttpResponsePtr LuaRoutes::executeRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params)
{
    LuaRequest luaRequest(req);
    LuaResponse luaResponse;
    const auto *chain = LuaMiddlewareManager::instance().get(method, path);

    if (chain && !chain->empty())
    {
        return executeMiddlewareChain(*chain, 0, luaRequest, luaResponse, handler, req, params);
    }

    return executeHandler(handler, req, params);
}

void LuaRoutes::registerAsyncRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    if (!handler.isTable() && !handler.isFunction())
        throw std::runtime_error("Async route requires a Lua table or function");

    switch (countPathParameters(path)) {
        case 0:
            registerAsyncRouteImpl<>(path, method, handler);
            break;

        case 1:
            registerAsyncRouteImpl<std::string>(path, method, handler);
            break;

        case 2:
            registerAsyncRouteImpl<std::string, std::string>(path, method, handler);
            break;

        case 3:
            registerAsyncRouteImpl<std::string, std::string, std::string>(path, method, handler);
            break;

        case 4:
            registerAsyncRouteImpl<std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        case 5:
            registerAsyncRouteImpl<std::string, std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        case 6:
            registerAsyncRouteImpl<std::string, std::string, std::string, std::string, std::string, std::string>(path, method, handler);
            break;

        default:
            throw std::runtime_error("Async routes may have at most 6 path parameters");
    }
}

void LuaRoutes::executeRouteAsync(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    const auto *chain = LuaMiddlewareManager::instance().get(method, path);
    if (chain && !chain->empty()) {
        executeLuaFunctionAsync(handler, req, params, std::move(callback), chain);
        return;
    }

    executeHandlerAsync(handler, req, params, std::move(callback));
}

void LuaRoutes::executeHandlerAsync(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    if (handler.isTable()) {
        Json::Value json = luaTableToJson(handler);
        callback(drogon::HttpResponse::newHttpJsonResponse(json));
        return;
    }

    if (handler.isFunction()) {
        executeLuaFunctionAsync(handler, req, params, std::move(callback));
        return;
    }

    throw std::runtime_error("Invalid Lua async route handler");
}

void LuaRoutes::executeLuaFunctionAsync(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const LuaMiddlewareManager::MiddlewareChain* middlewareChain) {
    if (!handler.isFunction())
        throw std::runtime_error("Lua async route handler is not a function");

    if (!req)
        throw std::runtime_error("Lua async route received null HttpRequest");

    lua_State* L = handler.state();

    if (!L)
        throw std::runtime_error("Lua async route handler has null Lua state");

    auto context = std::make_shared<LuaAsyncRouteContext>(L);
    context->handler = handler;
    context->request = std::make_unique<LuaRequest>(req);
    context->params = params;
    context->callback = std::move(callback);
    context->coroutine = LuaCoroutineManager::create(L);
    context->ownerLoop = trantor::EventLoop::getEventLoopOfCurrentThread();
    if (!context->coroutine)
        throw std::runtime_error("Failed to create Lua async coroutine");

    lua_State* co = LuaCoroutineManager::state(context->coroutine);
    if (!co)
        throw std::runtime_error("Lua async coroutine has null state");

    std::shared_ptr<LuaAsyncMiddlewareContext> middlewareContext;
    if (middlewareChain && !middlewareChain->empty()) {
        middlewareContext = std::make_shared<LuaAsyncMiddlewareContext>(L);
        middlewareContext->coroutine = context->coroutine;
        middlewareContext->callback = context->callback;
        middlewareContext->request = std::make_unique<LuaRequest>(req);
        middlewareContext->response = std::make_unique<LuaResponse>();
        middlewareContext->hasRouteResponse = false;
        middlewareContext->middlewareIndex = 0;
        middlewareContext->middlewareChain = middlewareChain;
        middlewareContext->handler = handler;
        middlewareContext->params = params;
    }

    LuaAsyncContextRegistry::set<LuaAsyncRouteContext>(context->coroutine->thread, context);
    if (middlewareContext)
        LuaAsyncContextRegistry::set<LuaAsyncMiddlewareContext>(context->coroutine->thread, middlewareContext);

    std::weak_ptr<LuaAsyncRouteContext> weakContext = context;
    std::weak_ptr<LuaAsyncMiddlewareContext> weakMiddlewareContext = middlewareContext;
    auto resumeCoroutine = [weakContext, weakMiddlewareContext]() {
        auto context = weakContext.lock();

        if (!context || !context->coroutine)
            return;

        auto middlewareContext = weakMiddlewareContext.lock();
        auto* ownerLoop = context->ownerLoop;

        if (!ownerLoop) {
            LOG_ERROR << "Lua async coroutine has no owner loop";
            return;
        }

        ownerLoop->queueInLoop([context, middlewareContext]() {
            if (!context || !context->coroutine)
                return;

            lua_State* co = LuaCoroutineManager::state(context->coroutine);
            if (!co)
                return;

            lua_pushboolean(co, 1);

            auto resumeResult = LuaCoroutineManager::resume(context->coroutine, 1);
            if (resumeResult.status == LuaCoroutineManager::Status::Error) {
                auto callback = std::move(context->callback);
                std::string error = "Lua async route handler failed: " + resumeResult.error;

                LuaRoutes::cleanupAsyncRoute(context, middlewareContext);
                if (callback)
                    LuaRoutes::sendErrorResponse(error, std::move(callback));

                return;
            }

            if (resumeResult.status == LuaCoroutineManager::Status::Yielded) {
                if (resumeResult.nresults == 1 && lua_isboolean(co, -1)) {
                    if (middlewareContext && middlewareContext->resume)
                        middlewareContext->resume();

                    else if (context->resume)
                        context->resume();
                }

                return;
            }

            if (resumeResult.status != LuaCoroutineManager::Status::Finished) {
                auto callback = std::move(context->callback);
                LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                if (callback)
                    LuaRoutes::sendErrorResponse("Unknown async coroutine state", std::move(callback));

                return;
            }

            // Middleware finished
            if (middlewareContext) {
                if (middlewareContext->hasRouteResponse && middlewareContext->response && middlewareContext->response->response()) {
                    auto callback = std::move(middlewareContext->callback);
                    auto response = middlewareContext->response->response();
                    LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                    if (callback)
                        callback(response);

                    return;
                }

                if (middlewareContext->response && middlewareContext->response->response()) {
                    auto callback = std::move(middlewareContext->callback);
                    auto response = middlewareContext->response->response();
                    LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                    if (callback)
                        callback(response);

                    return;
                }

                if (resumeResult.nresults < 1) {
                    auto callback = std::move(middlewareContext->callback);
                    LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                    if (callback)
                        LuaRoutes::sendErrorResponse("Async route handler must return a table or Drogua.Response", std::move(callback));

                    return;
                }

                try {
                    auto response = LuaRoutes::luaResultToResponse(co, -1);
                    auto callback = std::move(middlewareContext->callback);
                    LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                    if (callback)
                        callback(response);

                    return;
                }

                catch (const std::exception& e) {
                    auto callback = std::move(middlewareContext->callback);
                    LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

                    if (callback)
                        LuaRoutes::sendErrorResponse(e.what(), std::move(callback));

                    return;
                }

            }

            // Normal async route finished.
            if (resumeResult.nresults < 1) {
                auto callback = std::move(context->callback);
                LuaRoutes::cleanupAsyncRoute(context, nullptr);

                if (callback)
                    LuaRoutes::sendErrorResponse("Async route handler must return a table or Drogua.Response", std::move(callback));

                return;
            }

            try {
                auto response = LuaRoutes::luaResultToResponse(co, -1);
                auto callback = std::move(context->callback);
                LuaRoutes::cleanupAsyncRoute(context, nullptr);

                if (callback)
                    callback(response);

                return;
            }

            catch (const std::exception& e) {
                auto callback = std::move(context->callback);
                LuaRoutes::cleanupAsyncRoute(context, nullptr);

                if (callback)
                    LuaRoutes::sendErrorResponse(e.what(), std::move(callback));

                return;
            }
        });
    };

    context->resume = resumeCoroutine;
    if (middlewareContext)
        middlewareContext->resume = resumeCoroutine;

    int argumentCount = 0;
    if (middlewareContext) {
        pushAsyncMiddleware(middlewareContext);
        argumentCount = 3;
    }

    else {
        LuaCoroutineManager::pushFunction(context->coroutine, handler);
        auto pushResult = luabridge::Stack<LuaRequest*>::push(co, context->request.get());
        if (!pushResult) {
            LuaRoutes::cleanupAsyncRoute(context, nullptr);
            throw std::runtime_error("Failed to push LuaRequest: " + pushResult.message());
        }

        for (const auto& param : params)
            lua_pushlstring(co, param.data(), param.size());

        argumentCount = 1 + static_cast<int>(params.size());
    }

    auto resumeResult = LuaCoroutineManager::resume(context->coroutine, argumentCount);
    if (resumeResult.status == LuaCoroutineManager::Status::Yielded) {
        if (resumeResult.nresults == 1 && lua_isboolean(co, -1)) {
            if (middlewareContext && middlewareContext->resume)
                middlewareContext->resume();

            else if (context->resume)
                context->resume();
        }

        return;
    }

    if (resumeResult.status == LuaCoroutineManager::Status::Error) {
        std::string error = "Lua async route handler failed: " + resumeResult.error;
        LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

        throw std::runtime_error(error);
    }

    if (resumeResult.status != LuaCoroutineManager::Status::Finished) {
        LuaRoutes::cleanupAsyncRoute(context, middlewareContext);
        throw std::runtime_error("Unknown async coroutine state");
    }

    /* Middleware finished immediately. */
    if (middlewareContext) {
        if (middlewareContext->hasRouteResponse && middlewareContext->response && middlewareContext->response->response()) {
            auto callback = std::move(middlewareContext->callback);
            auto response = middlewareContext->response->response();
            LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

            if (callback)
                callback(response);

            return;
        }

        if (middlewareContext->response && middlewareContext->response->response()) {
            auto callback = std::move(middlewareContext->callback);
            auto response = middlewareContext->response->response();
            LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

            if (callback)
                callback(response);

            return;
        }

        if (resumeResult.nresults < 1) {
            LuaRoutes::cleanupAsyncRoute(context, middlewareContext);
            throw std::runtime_error("Lua async route handler must return a table or Drogua.Response");
        }

        try {
            auto response = LuaRoutes::luaResultToResponse(co, -1);
            auto callback = std::move(middlewareContext->callback);
            LuaRoutes::cleanupAsyncRoute(context, middlewareContext);

            if (callback)
                callback(response);

            return;
        }

        catch (...) {
            LuaRoutes::cleanupAsyncRoute(context, middlewareContext);
            throw;
        }

    }

    // Normal async route finished immediately. 
    if (resumeResult.nresults < 1) {
        LuaRoutes::cleanupAsyncRoute(context, nullptr);
        throw std::runtime_error("Lua async route handler must return a table or Drogua.Response");
    }

    try {
        auto response = LuaRoutes::luaResultToResponse(co, -1);
        auto callback = std::move(context->callback);
        LuaRoutes::cleanupAsyncRoute(context, nullptr);

        if (callback)
            callback(response);

        return;
    }

    catch (...) {
        LuaRoutes::cleanupAsyncRoute(context, nullptr);
        throw;
    }
}

void LuaRoutes::pushAsyncMiddleware(const std::shared_ptr<LuaAsyncMiddlewareContext>& context) {
    if (!context || !context->coroutine) {
        throw std::runtime_error("Invalid async middleware context");
    }

    lua_State* co = LuaCoroutineManager::state(context->coroutine);
    if (!co) {
        throw std::runtime_error("Async middleware coroutine has null state");
    }

    if (!context->middlewareChain || context->middlewareIndex >= context->middlewareChain->size()) {
        throw std::runtime_error("pushAsyncMiddleware called with no middleware");
    }

    LuaMiddleware* middleware = (*context->middlewareChain)[context->middlewareIndex];
    if (!middleware) {
        throw std::runtime_error("Async middleware chain contains a null middleware");
    }

    middleware->executeAsync(co, *context->request, *context->response);
}

void LuaRoutes::cleanupAsyncRoute(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext) {
    if (!context) return;

    lua_State* co = nullptr;
    if (context->coroutine) co = LuaCoroutineManager::state(context->coroutine);
    // Registry owns the contexts, so remove those references first.
    if (co) {
        LuaAsyncContextRegistry::clear<LuaAsyncRouteContext>(co);
        if (middlewareContext) LuaAsyncContextRegistry::clear<LuaAsyncMiddlewareContext>(co);

        // The coroutine must still exist while clearing the registry.
        LuaCoroutineManager::clearStack(co);
    }

    // Break callback references before releasing the contexts.
    context->resume = nullptr;
    context->callback = nullptr;

    if (middlewareContext) {
        middlewareContext->resume = nullptr;
        middlewareContext->callback = nullptr;
    }

    // Both contexts currently share ownership of the coroutine.
    context->coroutine.reset();
    if (middlewareContext) middlewareContext->coroutine.reset();
}

LuaRoutes::LuaRouteArgs LuaRoutes::parseRouteArgs(lua_State *L, const char *methodName) {
    const int argc = lua_gettop(L);
    if (argc < 2 || argc > 3) {
        throw std::runtime_error(std::string("Drogua.Routes.") + methodName + " expects 2 or 3 arguments");
    }

    const char *path = luaL_checkstring(L, 1);
    auto handler = luabridge::Stack<luabridge::LuaRef>::get(L, 2);
    if (!handler) {
        throw std::runtime_error("Invalid route handler: " + handler.message());
    }

    return {path, handler.value()};
}

void LuaRoutes::registerMiddleware(lua_State *L, int argc, drogon::HttpMethod method, const std::string &path) {
    if (argc != 3)
        return;

    if (!lua_istable(L, 3)) {
        throw std::runtime_error("Route middleware must be a table");
    }

    auto middleware = luabridge::Stack<luabridge::LuaRef>::get(L, 3);
    if (!middleware) {
        throw std::runtime_error("Invalid middleware table: " + middleware.message());
    }

    LuaMiddlewareManager::instance().add(method, path, middleware.value());
}

drogon::HttpResponsePtr LuaRoutes::luaResultToResponse(lua_State* L, int index) {
    auto result = luabridge::Stack<luabridge::LuaRef>::get(L, index);
    if (!result) {
        throw std::runtime_error(
            "Failed to retrieve Lua async route result: " +
            result.message());
    }

    const auto& luaResult = result.value();
    if (luaResult.isUserdata()) {
        auto response = luabridge::get<LuaResponse*>(L, index);

        if (response) {
            return response.value()->response();
        }
    }

    if (luaResult.isTable()) {
        try {
            Json::Value json = LuaRoutes::luaTableToJson(luaResult);
            return drogon::HttpResponse::newHttpJsonResponse(json);
        }

        catch (const std::exception& e) {
            throw std::runtime_error("Failed to convert Lua result to JSON: " + std::string(e.what()));
        }
    }

    throw std::runtime_error("Lua async route handler must return a table or Drogua.Response");
}