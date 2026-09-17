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
        // ==== Register Sync Routes =====
        static void registerRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);

        template <typename... Params>
        static void registerRouteImpl(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
            drogon::app().registerHandler(path, 
                [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, Params... params) {
                    try {
                        std::vector<std::string> parameters{std::string(params)...};
                        auto response = LuaRoutes::executeRoute(path, method, handler, req, parameters);
                        callback(std::move(response));
                    }

                    catch (const std::exception& e) {
                        LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
                    }
                },
                {method}
            );
        }

        // ==== Register Async Routes =====
        static int luaRegisterAsync(lua_State *L, drogon::HttpMethod method, const char *methodName);
        static void registerAsyncRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler);

        template <typename... Params>
        static void registerAsyncRouteImpl(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler) {
            drogon::app().registerHandler(path,
                [handler, method, path](const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, Params... params) {
                    try {
                        std::vector<std::string> parameters{std::string(params)...};
                        LuaRoutes::executeRouteAsync(path, method, handler, req, parameters, std::move(callback));
                    }

                    catch (const std::exception& e) {
                        LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
                    }
                },
                {method}
            );
        }

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

        static void pushAsyncMiddleware(const std::shared_ptr<LuaAsyncMiddlewareContext>& context);

        static int luaRegister(lua_State *L, drogon::HttpMethod method, const char *methodName);

        struct LuaRouteArgs {
            std::string path;
            luabridge::LuaRef handler;
        };

        static LuaRouteArgs parseRouteArgs(lua_State *L, const char *methodName);
        static void registerMiddleware(lua_State *L, int argc, drogon::HttpMethod method, const std::string &path);
        static drogon::HttpResponsePtr luaResultToResponse(lua_State* L, int index);
        static void finishAsyncRoute(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext, const drogon::HttpResponsePtr& response);
        static void failAsyncRoute(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext, const std::string& message);
        static std::shared_ptr<LuaAsyncRouteContext> createAsyncContext(lua_State* L, const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
        static std::shared_ptr<LuaAsyncMiddlewareContext> createMiddlewareContext(lua_State* L, const std::shared_ptr<LuaAsyncRouteContext>& context, const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req, const std::vector<std::string>& params, const LuaMiddlewareManager::MiddlewareChain* middlewareChain);
        static void setupAsyncRegistry(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
        static void resumeAsyncRoute(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
        static bool handleAsyncYield(const LuaCoroutineManager::ResumeResult& result, lua_State* co, const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
        static bool handleMiddlewareResponse(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
        static void handleAsyncFinished(const LuaCoroutineManager::ResumeResult& result, lua_State* co, const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
        static void cleanupAsyncRoute(const std::shared_ptr<LuaAsyncRouteContext>& context, const std::shared_ptr<LuaAsyncMiddlewareContext>& middlewareContext);
};
