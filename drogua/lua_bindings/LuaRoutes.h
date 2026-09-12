#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "LuaMiddlewareManager.h"
#include "LuaRequest.h"
#include "LuaResponse.h"
#include "LuaCoroutineManager.h"
#include "LuaDatabase.h"
#include "LuaResult.h"
#include "LuaAsyncContext.h"
#include "LuaAsyncContextRegistry.h"
#include "LuaMiddleware.h"

#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <chrono>

class LuaRoutes {
    public:
        static int luaGet(lua_State *L);
        static int luaPost(lua_State *L);
        static int luaPut(lua_State *L);
        static int luaDelete(lua_State *L);
        static int luaPatch(lua_State *L);

        static int luaGetAsync(lua_State *L);
        static int luaPostAsync(lua_State *L);
        static int luaPutAsync(lua_State *L);
        static int luaDeleteAsync(lua_State *L);
        static int luaPatchAsync(lua_State *L);

        static Json::Value luaTableToJson(const luabridge::LuaRef &table);
        static Json::Value luaValueToJson(lua_State *L, int index);

    private:
        // route registration router to the other methods
        static void registerRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);

        // == register route for 0..6 path parameters methods
        static void registerRoute0(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute1(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute2(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute3(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute4(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute5(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerRoute6(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        // == end of this part

        // ==== Register Async Routes =====
        static int luaRegisterAsync(lua_State *L, drogon::HttpMethod method, const char *methodName);
        static void registerAsyncRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerAsyncRoute0(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);
        static void registerAsyncRoute1(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerAsyncRoute2(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerAsyncRoute3(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerAsyncRoute4(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerAsyncRoute5(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerAsyncRoute6(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        // ================================

        static drogon::HttpResponsePtr executeHandler(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params);
        static drogon::HttpResponsePtr executeLuaTable(const luabridge::LuaRef &handler);
        static drogon::HttpResponsePtr executeLuaFunction(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params);

        // =============== Asynchronous execution =====================
        static void executeHandlerAsync(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        static void executeLuaFunctionAsync(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params, std::function<void(const drogon::HttpResponsePtr&)>&& callback, const LuaMiddlewareManager::MiddlewareChain* middlewareChain = nullptr);
        static void executeRouteAsync(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        static void cleanupAsyncContext(const std::shared_ptr<LuaAsyncContext>& context);
        static void resumeAsyncRoute(const std::shared_ptr<LuaAsyncContext>& context, const std::function<void(lua_State*)>& pushValue);
        // ============================================================

        static void sendJsonResponse(const Json::Value &json, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        static void sendErrorResponse(const std::string &message, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        static size_t countPathParameters(const std::string &path);

        static drogon::HttpResponsePtr executeRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler, 
            const drogon::HttpRequestPtr &req, const std::vector<std::string> &params);

        static drogon::HttpResponsePtr executeMiddlewareChain(const LuaMiddlewareManager::MiddlewareChain &chain, std::size_t index, LuaRequest &req, 
            LuaResponse &res, const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &httpReq, const std::vector<std::string> &params);

        static void pushAsyncMiddleware(const std::shared_ptr<LuaAsyncContext>& context);

        static int luaRegister(lua_State *L, drogon::HttpMethod method, const char *methodName);
};
