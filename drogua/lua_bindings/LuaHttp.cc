#include "LuaHttp.h"
#include "LuaHttpResult.h"
#include "LuaAsyncContextRegistry.h"
#include "LuaAsyncContext.h"
#include "LuaRoutes.h"
#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>
#include <stdexcept>
#include <chrono>
#include <mutex>

int LuaHttp::requestAsync(lua_State* L) {
    if (!L) {
        return luaL_error(L, "Http.requestAsync received null lua_State");
    }

    auto routeContext = LuaAsyncContextRegistry::get<LuaAsyncRouteContext>(L);
    if (!routeContext) {
        return luaL_error(L, "Http.requestAsync must be called from an async route");
    }

    if (!routeContext->coroutine) {
        return luaL_error(L, "Http.requestAsync has no active coroutine");
    }

    if (!routeContext->ownerLoop) {
        return luaL_error(L, "Http.requestAsync has no owner event loop");
    }

    auto* loop = routeContext->ownerLoop;
    const char* url = luaL_checkstring(L, 1);
    luabridge::LuaRef options = luabridge::LuaRef::fromStack(L, 2);

    auto httpContext = std::make_shared<LuaAsyncHttpContext>();
    httpContext->coroutine = routeContext->coroutine;
    httpContext->resume = routeContext->resume;
    httpContext->callback = routeContext->callback;
    httpContext->ownerLoop = loop;
    httpContext->request = routeContext->request;
    httpContext->url = url;
    httpContext->retryCount = 0;
    httpContext->maxRetries = 0;
    httpContext->retryDelayMs = 100;

    if (options.isTable()) {
        auto retry = options["retry"];
        if (retry.isTable()) {
            auto count = retry["count"];
            if (count.isNumber()) {
                httpContext->maxRetries = static_cast<int>(count.cast<lua_Integer>().value());
            }

            auto delay = retry["delay"];
            if (delay.isNumber()) {
                httpContext->retryDelayMs = static_cast<int>(delay.cast<lua_Integer>().value());
            }
        }
    }

    httpContext->httpRequest = buildRequest(L, url, options);
    if (!httpContext->httpRequest) {
        return luaL_error(L, "Failed to build HTTP request");
    }

    LuaAsyncContextRegistry::set<LuaAsyncHttpContext>(L, httpContext);
    requestAsync(
        url,
        options,
        loop,
        [httpContext](std::shared_ptr<LuaHttpResult> result) {
            if (httpContext->request && !httpContext->request->connected()) {
                return;
            }

            if (!result) {
                httpContext->asyncResult.reset();
                httpContext->asyncError = "HTTP request returned null result";
                if (httpContext->resume) {
                    httpContext->resume();
                }

                return;
            }

            httpContext->asyncResult = std::move(result);
            httpContext->asyncError.clear();
            if (httpContext->resume) {
                httpContext->resume();
            }
        },
        [httpContext, options, loop](const std::string& error) {
            if (httpContext->request && !httpContext->request->connected()) {
                return;
            }

            if (httpContext->retryCount < httpContext->maxRetries) {
                ++httpContext->retryCount;
                scheduleRetry(
                    nullptr,
                    httpContext->url,
                    options,
                    loop,
                    [httpContext](std::shared_ptr<LuaHttpResult> result) {
                        if (httpContext->request && !httpContext->request->connected()) {
                            return;
                        }

                        httpContext->asyncResult = std::move(result);
                        httpContext->asyncError.clear();
                        if (httpContext->resume) {
                            httpContext->resume();
                        }
                    },

                    [httpContext](const std::string& retryError) {
                        if (httpContext->request && !httpContext->request->connected()) {
                            return;
                        }

                        httpContext->asyncResult.reset();
                        httpContext->asyncError = retryError;
                        if (httpContext->resume) {
                            httpContext->resume();
                        }
                    },
                    httpContext->retryCount,
                    httpContext->maxRetries,
                    httpContext->retryDelayMs);

                return;
            }

            httpContext->asyncResult.reset();
            httpContext->asyncError = error;

            if (httpContext->resume) {
                httpContext->resume();
            }
        },
        httpContext->maxRetries,
        httpContext->retryDelayMs);

    return lua_yieldk(L, 0, 0, &LuaHttp::requestAsyncContinuation);
}

void LuaHttp::requestAsync(const std::string& url, const luabridge::LuaRef& options, trantor::EventLoop* loop, std::function<void(std::shared_ptr<LuaHttpResult>)> callback, std::function<void(const std::string&)> errorCallback, int maxRetries, int retryDelayMs) {
    (void)maxRetries;
    (void)retryDelayMs;
    if (!callback) {
        throw std::runtime_error("Http async callback is empty");
    }

    if (!errorCallback) {
        throw std::runtime_error("Http async error callback is empty");
    }

    if (!loop) {
        throw std::runtime_error("Http async request has no event loop");
    }

    try {
        auto req = buildRequest(nullptr, url, options);
        if (!req) {
            errorCallback("Failed to build HTTP request");
            return;
        }

        auto client = getClient(url, loop);
        if (!client) {
            errorCallback("Failed to create HTTP client");
            return;
        }

        client->sendRequest(
            req,
            [callback, errorCallback](drogon::ReqResult result, const drogon::HttpResponsePtr& resp) {
                if (result == drogon::ReqResult::Ok && resp) {
                    callback(std::make_shared<LuaHttpResult>(resp));
                    return;
                }

                std::string error = "Request failed: " + std::string(drogon::to_string_view(result));
                if (resp) {
                    error += " (Status: " + std::to_string(static_cast<int>(resp->statusCode())) + ")";
                }
                errorCallback(error);
            });
    }

    catch (const std::exception& e) {
        errorCallback(e.what());
    }

    catch (...) {
        errorCallback("Unknown exception starting HTTP request");
    }
}

int LuaHttp::requestAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;
    if (!L) return 0;

    auto httpContext = LuaAsyncContextRegistry::get<LuaAsyncHttpContext>(L);
    if (!httpContext) return luaL_error(L, "Http continuation has no HTTP context");

    if (status != LUA_YIELD) {
        LuaAsyncContextRegistry::clear<LuaAsyncHttpContext>(L);
        return luaL_error(L, "Http continuation resumed with unexpected status");
    }

    if (!httpContext->asyncError.empty()) {
        std::string error = std::move(httpContext->asyncError);
        LuaAsyncContextRegistry::clear<LuaAsyncHttpContext>(L);
        return luaL_error(L, "HTTP request failed: %s", error.c_str());
    }

    if (!httpContext->asyncResult) {
        LuaAsyncContextRegistry::clear<LuaAsyncHttpContext>(L);
        return luaL_error(L, "HTTP request completed without response");
    }

    auto result = std::move(httpContext->asyncResult);
    LuaAsyncContextRegistry::clear<LuaAsyncHttpContext>(L);

    auto pushResult = luabridge::Stack<std::shared_ptr<LuaHttpResult>>::push(L, result);
    if (!pushResult) return luaL_error(L, "Failed to push HTTP result: %s", pushResult.message().c_str());

    return 1;
}

void LuaHttp::scheduleRetry(lua_State* L, const std::string& url, const luabridge::LuaRef& options, trantor::EventLoop* loop, std::function<void(std::shared_ptr<LuaHttpResult>)> callback, std::function<void(const std::string&)> errorCallback, int retryCount, int maxRetries, int retryDelayMs) {
    (void)L;
    if (!loop) {
        errorCallback("No event loop available for retry");
        return;
    }

    const int delay = retryDelayMs * (1 << (retryCount - 1));
    loop->runAfter(
        std::chrono::milliseconds(delay),
        [url, options, loop, callback, errorCallback, retryCount, maxRetries, retryDelayMs]() {
            auto req = buildRequest(nullptr, url, options);
            if (!req) {
                errorCallback("Failed to rebuild HTTP request for retry");
                return;
            }

            auto client = getClient(url, loop);
            client->sendRequest(
                req,
                [callback, errorCallback](drogon::ReqResult result, const drogon::HttpResponsePtr& resp) {
                    if (result == drogon::ReqResult::Ok && resp) {
                        callback(std::make_shared<LuaHttpResult>(resp));
                        return;
                    }

                    std::string errMsg = "Retry failed: " + std::string(drogon::to_string_view(result));
                    if (resp) errMsg += " (Status: " + std::to_string(static_cast<int>(resp->statusCode())) + ")";
                    errorCallback(errMsg);
                });
        });
}

drogon::HttpRequestPtr LuaHttp::buildRequest(lua_State* L, const std::string& url, const luabridge::LuaRef& options) {
    auto req = drogon::HttpRequest::newHttpRequest();
    std::string path;
    const size_t schemeEnd = url.find("://");
    if (schemeEnd != std::string::npos) {
        const size_t authorityStart = schemeEnd + 3;
        const size_t pathStart = url.find('/', authorityStart);
        if (pathStart != std::string::npos) {
            path = url.substr(pathStart);
        }

        else {
            path = "/";
        }
    }

    else {
        path = url.empty() ? "/" : url;
        if (path[0] != '/') {
            path.insert(path.begin(), '/');
        }
    }

    req->setPath(path);
    lua_State* state = L ? L : options.state();
    if (!state || !options.isTable()) {
        return req;
    }

    luabridge::LuaRef methodRef = options["method"];
    if (methodRef.isString()) {
        std::string method = methodRef.cast<std::string>().value();
        if (method == "POST" || method == "post") req->setMethod(drogon::Post);
        else if (method == "PUT" || method == "put") req->setMethod(drogon::Put);
        else if (method == "DELETE" || method == "delete") req->setMethod(drogon::Delete);
        else if (method == "PATCH" || method == "patch") req->setMethod(drogon::Patch);
        else if (method == "HEAD" || method == "head") req->setMethod(drogon::Head);
        else if (method == "OPTIONS" || method == "options") req->setMethod(drogon::Options);
        else req->setMethod(drogon::Get);
    }

    luabridge::LuaRef headersRef = options["headers"];
    if (headersRef.isTable()) {
        headersRef.push(state);
        lua_pushnil(state);
        while (lua_next(state, -2) != 0) {
            if (lua_isstring(state, -2) && lua_isstring(state, -1)) {
                const char* key = lua_tostring(state, -2);
                const char* value = lua_tostring(state, -1);
                if (key && value) {
                    req->addHeader(key, value);
                }
            }
            lua_pop(state, 1);
        }
        lua_pop(state, 1);
    }

    luabridge::LuaRef bodyRef = options["body"];
    if (bodyRef.isString()) {
        req->setBody(bodyRef.cast<std::string>().value());
    }

    else if (bodyRef.isTable()) {
        try {
            auto json = LuaRoutes::luaTableToJson(bodyRef);
            req->setBody(json.toStyledString());
            req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        }

        catch (...) {
            req->setBody("");
        }
    }

    if (headersRef.isTable()) {
        luabridge::LuaRef contentTypeRef = headersRef["Content-Type"];
        if (contentTypeRef.isString()) {
            std::string contentType = contentTypeRef.cast<std::string>().value();
            if (contentType.find("application/json") != std::string::npos) {
                req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            }
        }
    }

    return req;
}

std::shared_ptr<drogon::HttpClient> LuaHttp::getClient(const std::string& baseUrl, trantor::EventLoop* loop) {
    static std::unordered_map<std::string, std::shared_ptr<drogon::HttpClient>> clientCache;
    static std::mutex cacheMutex;
    std::string key = baseUrl;
    const size_t schemeEnd = baseUrl.find("://");
    size_t authorityStart = 0;
    if (schemeEnd != std::string::npos) {
        authorityStart = schemeEnd + 3;
    }

    const size_t pathStart = baseUrl.find('/', authorityStart);
    if (pathStart != std::string::npos) {
        key = baseUrl.substr(authorityStart, pathStart - authorityStart);
    }

    else {
        key = baseUrl.substr(authorityStart);
    }

    std::string host = key;
    uint16_t port = 80;
    const size_t portPos = key.rfind(':');
    if (portPos != std::string::npos) {
        host = key.substr(0, portPos);
        const int parsedPort = std::stoi(key.substr(portPos + 1));
        if (parsedPort <= 0 || parsedPort > 65535) {
            throw std::runtime_error("LuaHttp: invalid port: " + std::to_string(parsedPort));
        }
        port = static_cast<uint16_t>(parsedPort);
    }

    const bool useSSL = baseUrl.rfind("https://", 0) == 0;
    trantor::EventLoop* clientLoop = loop ? loop : drogon::app().getLoop();
    if (!clientLoop) {
        throw std::runtime_error("LuaHttp: Drogon application loop is null");
    }

    const std::string cacheKey = key + "|" + std::to_string(reinterpret_cast<uintptr_t>(clientLoop));
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = clientCache.find(cacheKey);
    if (it != clientCache.end()) {
        return it->second;
    }

    auto client = drogon::HttpClient::newHttpClient(host, port, useSSL, clientLoop);
    if (!client) {
        throw std::runtime_error("LuaHttp: failed to create HttpClient");
    }
    clientCache.emplace(cacheKey, client);

    return client;
}