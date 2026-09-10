#include "LuaRoutes.h"
#include "LuaRequest.h"
#include "LuaResponse.h"
#include "LuaMiddlewareManager.h"

#include <stdexcept>
#include <string>

using namespace drogon;

int LuaRoutes::luaGet(lua_State* L) {
    return luaRegister(L, Get, "get");
}

int LuaRoutes::luaPost(lua_State* L) {
    return luaRegister(L, Post, "post");
}

int LuaRoutes::luaPut(lua_State* L) {
    return luaRegister(L, Put, "put");
}

int LuaRoutes::luaDelete(lua_State* L) {
    return luaRegister(L, Delete, "delete");
}

int LuaRoutes::luaPatch(lua_State* L) {
    return luaRegister(L, Patch, "patch");
}

// Async Routes
int LuaRoutes::luaGetAsync(lua_State* L) {
    return luaRegisterAsync(L, Get, "getAsync");
}

int LuaRoutes::luaPostAsync(lua_State* L) {
    return luaRegisterAsync(L, Post, "postAsync");
}

int LuaRoutes::luaPutAsync(lua_State* L) {
    return luaRegisterAsync(L, Put, "putAsync");
}

int LuaRoutes::luaDeleteAsync(lua_State* L) {
    return luaRegisterAsync(L, Delete, "deleteAsync");
}

int LuaRoutes::luaPatchAsync(lua_State* L) {
    return luaRegisterAsync(L, Patch, "patchAsync");
}

int LuaRoutes::luaRegister(lua_State* L, drogon::HttpMethod method, const char* methodName) {
    const int argc = lua_gettop(L);

    if (argc < 2 || argc > 3)
        return luaL_error(L, "Drogua.Routes.%s expects 2 or 3 arguments", methodName);

    // Argument 1: path
    const char* path = luaL_checkstring(L, 1);

    // Argument 2: handler
    auto handler = luabridge::Stack<luabridge::LuaRef>::get(L, 2);
    if (!handler)
        return luaL_error(L, "Invalid route handler: %s", handler.message().c_str());

    try {
        // Argument 3: optional middleware table
        if (argc == 3) {
            if (!lua_istable(L, 3))
                return luaL_error(L, "Route middleware must be a table");

            auto middleware = luabridge::Stack<luabridge::LuaRef>::get(L, 3);
            if (!middleware)
                return luaL_error(L, "Invalid middleware table: %s", middleware.message().c_str());

            LuaMiddlewareManager::instance().add(method, path, middleware.value());
        }

        registerRoute(path, method, handler.value());
    } catch (const std::exception& e) {
        return luaL_error(L, "%s", e.what());
    }

    return 0;
}

Json::Value LuaRoutes::luaValueToJson(lua_State* L, int index) {
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

        case LUA_TTABLE: {
            luabridge::LuaRef nested = luabridge::LuaRef::fromStack(L, index);

            return luaTableToJson(nested);
        }

        default:
            std::cerr << "Unsupported Lua value\n";
            return Json::nullValue;
    }
}

Json::Value LuaRoutes::luaTableToJson(const luabridge::LuaRef& table) {
    if (!table.isTable()) return Json::nullValue;

    lua_State* L = table.state();

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

            } else {
                isArray = false;
            }
        } else {
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
            const char* key = lua_tostring(L, -2);
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

    switch (const size_t count = countPathParameters(path)) {
        case 0:
            registerRoute0(path, method, handler);
            break;
        case 1:
            registerRoute1(path, method, handler);
            break;
        case 2:
            registerRoute2(path, method, handler);
            break;
        case 3:
            registerRoute3(path, method, handler);
            break;
        case 4:
            registerRoute4(path, method, handler);
            break;
        case 5:
            registerRoute5(path, method, handler);
            break;
        case 6:
            registerRoute6(path, method, handler);
            break;
        default:
            throw std::runtime_error("Routes may have at most 6 path parameters");
    }
}

drogon::HttpResponsePtr LuaRoutes::executeHandler(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params) {
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

drogon::HttpResponsePtr LuaRoutes::executeLuaFunction(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params) {
    lua_State* L = handler.state();
    const int base = lua_gettop(L);
    LuaRequest luaRequest(req);

    handler.push(L);

    auto pushResult = luabridge::Stack<LuaRequest*>::push(L, &luaRequest);
    if (!pushResult) {
        lua_settop(L, base);
        throw std::runtime_error("Failed to push LuaRequest: " + pushResult.message());
    }

    for (const auto& param : params)
        lua_pushlstring(L, param.data(), param.size());

    const int status = lua_pcall(L, 1 + static_cast<int>(params.size()), 1, 0);
    if (status != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        lua_settop(L, base);
        throw std::runtime_error(
            "Lua route handler failed: " +
            std::string(error ? error : "Unknown Lua error"));
    }

    auto result = luabridge::Stack<luabridge::LuaRef>::get(L, -1);
    if (!result) {
        lua_settop(L, base);
        throw std::runtime_error(
            "Failed to retrieve Lua route result: " + result.message());
    }

    auto luaResult = result.value();

    // LuaResponse
    if (luaResult.isUserdata()) {
        //auto response = luabridge::get<LuaResponse>(L, -1);
        auto response = luabridge::get<LuaResponse*>(L, -1);

        if (response) {
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

size_t LuaRoutes::countPathParameters(const std::string& path) {
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


drogon::HttpResponsePtr LuaRoutes::executeMiddlewareChain(const LuaMiddlewareManager::MiddlewareChain& chain, std::size_t index, LuaRequest& req, LuaResponse& res, const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& httpReq, const std::vector<std::string>& params) {
    /*
     * No more middleware.
     * Execute the actual route handler.
     */
    if (index >= chain.size())
        return executeHandler(handler, httpReq, params);

    LuaMiddleware* middleware = chain[index];

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


drogon::HttpResponsePtr LuaRoutes::executeRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params) {
    LuaRequest luaRequest(req);
    LuaResponse luaResponse;
    const auto* chain = LuaMiddlewareManager::instance().get(method, path);

    if (chain && !chain->empty()) {
        return executeMiddlewareChain(*chain, 0, luaRequest, luaResponse, handler, req, params);
    }

    return executeHandler(handler, req, params);
}



// ==
// ==
// == register route for 1..6 path parameters methods

void LuaRoutes::registerRoute0(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute1(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute2(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1, std::string p2) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1, p2});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute3(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1, std::string p2, std::string p3) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1, p2, p3});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute4(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1, std::string p2, std::string p3, std::string p4) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1, p2, p3, p4});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute5(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1, std::string p2, std::string p3, std::string p4, std::string p5) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1, p2, p3, p4, p5});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

void LuaRoutes::registerRoute6(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
    app().registerHandler(path, [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string p1, std::string p2, std::string p3, std::string p4, std::string p5, std::string p6) {
        std::cerr << "PATH: " << req->path() << '\n';
        for (const auto& [key, value] : req->getParameters())
            std::cerr << "PARAM: " << key << " = " << value << '\n';

        try {
            auto response = LuaRoutes::executeRoute(path, method, handler, req, {p1, p2, p3, p4, p5, p6});
            callback(std::move(response));
        } catch (const std::exception& e) {
            LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
        }
    }, {method});
}

// == end of this part 

int LuaRoutes::luaRegisterAsync(lua_State* L, drogon::HttpMethod method, const char* methodName) {
    const int argc = lua_gettop(L);

    if (argc < 2 || argc > 3) {
        return luaL_error(L, "Drogua.Routes.%s expects 2 or 3 arguments", methodName);
    }

    // Argument 1: path
    const char* path = luaL_checkstring(L, 1);

    // Argument 2: handler
    auto handler = luabridge::Stack<luabridge::LuaRef>::get(L, 2);

    if (!handler) {
        return luaL_error(L, "Invalid async route handler: %s", handler.message().c_str());
    }

    try {
        // Async routes currently do not support path parameters.
        if (countPathParameters(path) != 0) {
            return luaL_error(L, "Async routes currently support 0 path parameters only");
        }

        // Argument 3: optional middleware.
        //
        // Async middleware is intentionally not supported yet.
        if (argc == 3) {
            if (!lua_istable(L, 3)) {
                return luaL_error(L, "Route middleware must be a table");
            }

            auto middleware = luabridge::Stack<luabridge::LuaRef>::get(L, 3);

            if (!middleware) {
                return luaL_error(L, "Invalid middleware table: %s", middleware.message().c_str());
            }

            (void)middleware;

            return luaL_error(L, "Async middleware is not supported yet");
        }

        registerAsyncRoute(path, method, handler.value());
    }

    catch (const std::exception& e) {
        return luaL_error(L, "%s", e.what());
    }

    return 0;
}


void LuaRoutes::registerAsyncRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler) {
    if (!handler.isTable() && !handler.isFunction()) {
        throw std::runtime_error("Async route requires a Lua table or function");
    }

    const size_t count = countPathParameters(path);

    if (count != 0) {
        throw std::runtime_error("Async routes currently support 0 path parameters only");
    }

    registerAsyncRoute0(path, method, handler);
}

void LuaRoutes::registerAsyncRoute0(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler) {
    app().registerHandler(
        path,
        [handler, method, path](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            try {
                LuaRoutes::executeRouteAsync(path, method, handler, req, {}, std::move(callback));
            } catch (const std::exception &e) {
                LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
            }
        },
        {method});
}

void LuaRoutes::executeRouteAsync(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    // Middleware is intentionally not part of this first step.
    const auto *chain = LuaMiddlewareManager::instance().get(method, path);

    if (chain && !chain->empty()) {
        throw std::runtime_error("Async middleware is not supported yet");
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

void LuaRoutes::executeLuaFunctionAsync(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

    if (!handler.isFunction()) {
        throw std::runtime_error("Lua async route handler is not a function");
    }

    if (!req) {
        throw std::runtime_error("Lua async route received null HttpRequest");
    }

    auto context = std::make_shared<LuaAsyncContext>();

    context->httpRequest = req;
    context->request = std::make_unique<LuaRequest>(req);
    context->params = params;
    context->callback = std::move(callback);

    lua_State* L = handler.state();

    if (!L) {
        throw std::runtime_error("Lua async route handler has null Lua state");
    }

    // Create coroutine
    context->coroutine = LuaCoroutineManager::create(L);

    if (!context->coroutine) {
        throw std::runtime_error("Failed to create Lua async coroutine");
    }

    lua_State* co = LuaCoroutineManager::state(context->coroutine);

    if (!co) {
        throw std::runtime_error("Lua async coroutine has null state");
    }

    /*
     * Register coroutine -> async context.
     *
     * queryAsyncLua() uses this to find the context
     * associated with the currently executing coroutine.
     */
    LuaAsyncContextRegistry::set(context->coroutine->thread, context);

    /*
     * Called by the DB callback when the async operation
     * has completed.
     *
     * We schedule the actual Lua resume onto Drogon's
     * event loop rather than resuming Lua directly from
     * the DB callback.
     */
    context->resume = [context]() {
        if (!context || !context->coroutine) {
            return;
        }

        app().getLoop()->queueInLoop([context]() {
            if (!context || !context->coroutine) return;

            lua_State* co = LuaCoroutineManager::state(context->coroutine);
            if (!co) return;

            /*
             * queryAsyncContinuation() gets the actual DB result/error from LuaAsyncContext.
             * The argument itself is not used, so push a dummy value.
             */
            lua_pushboolean(co, 1);

            auto resumeResult = LuaCoroutineManager::resume(context->coroutine, 1);

            if (resumeResult.status == LuaCoroutineManager::Status::Error) {
                if (context->callback) {
                    LuaRoutes::sendErrorResponse("Lua async route handler failed: " + resumeResult.error, std::move(context->callback));
                }

                return;
            }

            if (resumeResult.status == LuaCoroutineManager::Status::Yielded) {
                if (context->callback) {
                    LuaRoutes::sendErrorResponse("Async route yielded more than once", std::move(context->callback));
                }

                return;
            }

            if (resumeResult.status != LuaCoroutineManager::Status::Finished) {
                if (context->callback) {
                    LuaRoutes::sendErrorResponse("Unknown async coroutine state", std::move(context->callback));
                }

                return;
            }

            if (resumeResult.nresults < 1) {
                if (context->callback) {
                    LuaRoutes::sendErrorResponse("Async route handler must return a table or Drogua.Response", std::move(context->callback));
                }

                return;
            }

            auto result = luabridge::Stack<luabridge::LuaRef>::get(co, -1);

            if (!result) {
                if (context->callback) {
                    LuaRoutes::sendErrorResponse("Failed to retrieve Lua async route result: " + result.message(), std::move(context->callback));
                }

                return;
            }

            auto luaResult = result.value();

            // Drogua.Response / LuaResponse
            if (luaResult.isUserdata()) {
                auto response = luabridge::get<LuaResponse*>(co, -1);
                if (response) {
                    context->callback(response.value()->response());

                    return;
                }
            }

            // Normal Lua table -> JSON
            if (luaResult.isTable()) {
                try {
                    Json::Value json = LuaRoutes::luaTableToJson(luaResult);
                    context->callback(drogon::HttpResponse::newHttpJsonResponse(json));

                    return;
                }

                catch (const std::exception& e) {
                    if (context->callback) {
                        LuaRoutes::sendErrorResponse("Failed to convert Lua result to JSON: " + std::string(e.what()), std::move(context->callback));
                    }

                    return;
                }
            }

            if (context->callback) {
                LuaRoutes::sendErrorResponse("Async route handler must return a table or Drogua.Response", std::move(context->callback));
            }
        });
    };

    // Push handler
    LuaCoroutineManager::pushFunction(context->coroutine, handler);

    // Push LuaRequest
    auto pushResult = luabridge::Stack<LuaRequest*>::push(co, context->request.get());

    if (!pushResult) {
        throw std::runtime_error("Failed to push LuaRequest: " + pushResult.message());
    }

    // Push route parameters
    for (const auto& param : params) {
        lua_pushlstring(co, param.data(), param.size());
    }

    // Initial coroutine execution
    const int argumentCount = 1 + static_cast<int>(params.size());
    auto resumeResult = LuaCoroutineManager::resume(context->coroutine, argumentCount);

    /*
     * Normal async path:
     *     db:queryAsync() -> lua_yieldk() -> Yielded -> return to Drogon
     * 
     * The DB callback will eventually call context->resume().
     */
    if (resumeResult.status == LuaCoroutineManager::Status::Yielded) {
        return;
    }

    // Lua failed before yielding
    if (resumeResult.status == LuaCoroutineManager::Status::Error) {
        throw std::runtime_error("Lua async route handler failed: " + resumeResult.error);
    }

    // Synchronous completion
    if (resumeResult.status != LuaCoroutineManager::Status::Finished) {
        throw std::runtime_error("Unknown async coroutine state");
    }

    if (resumeResult.nresults < 1) {
        throw std::runtime_error("Lua async route handler must return a table or Drogua.Response");
    }

    auto result = luabridge::Stack<luabridge::LuaRef>::get(co, -1);
    if (!result) {
        throw std::runtime_error("Failed to retrieve Lua async route result: " + result.message());
    }

    auto luaResult = result.value();

    // LuaResponse
    if (luaResult.isUserdata()) {
        auto response = luabridge::get<LuaResponse*>(co, -1);
        if (response) {
            context->callback(response.value()->response());

            return;
        }
    }

    // Lua Table
    if (luaResult.isTable()) {
        Json::Value json = LuaRoutes::luaTableToJson(luaResult);
        context->callback(drogon::HttpResponse::newHttpJsonResponse(json));

        return;
    }

    throw std::runtime_error("Lua async route handler must return a table or Drogua.Response");
}
