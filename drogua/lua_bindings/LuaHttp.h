// LuaHttp.h
#pragma once

#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class LuaAsyncRouteContext;
struct LuaAsyncHttpContext;
class LuaHttpResult;

class LuaHttp {
    public:
        static int requestAsync(lua_State* L);
        static int requestAsyncContinuation(lua_State* L, int status, lua_KContext ctx);
        static void requestAsync(const std::string& url, const luabridge::LuaRef& options, trantor::EventLoop* loop, std::function<void(std::shared_ptr<LuaHttpResult>)> callback, std::function<void(const std::string&)> errorCallback, int maxRetries, int retryDelayMs);
        // create/reuse HttpClient
        static std::shared_ptr<drogon::HttpClient> getClient(const std::string& baseUrl, trantor::EventLoop* loop = nullptr);

    private:
        static drogon::HttpRequestPtr buildRequest(lua_State* L, const std::string& url, const luabridge::LuaRef& options);
        static void scheduleRetry(lua_State* L, const std::string& url, const luabridge::LuaRef& options, trantor::EventLoop* loop, std::function<void(std::shared_ptr<LuaHttpResult>)> callback, std::function<void(const std::string&)> errorCallback, int retryCount, int maxRetries, int retryDelayMs);
        static void clearAsyncHttpContext(lua_State* L);
};
