#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "LuaMiddlewareManager.h"
#include "LuaRequest.h"
#include "LuaResponse.h"

#include <vector>

class LuaRoutes {
    public:
        static int luaGet(lua_State* L);
        static int luaPost(lua_State* L);
        static int luaPut(lua_State* L);
        static int luaDelete(lua_State* L);
        static int luaPatch(lua_State* L);

        static Json::Value luaTableToJson(const luabridge::LuaRef& table);
        static Json::Value luaValueToJson(lua_State* L, int index);

    private:
        // route registration router to the other methods
        static void registerRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        // == register route for 0..6 path parameters methods
        static void registerRoute0(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute1(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute2(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute3(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute4(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute5(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        static void registerRoute6(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);
        // == end of this part 

        static drogon::HttpResponsePtr executeHandler(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr &req, const std::vector<std::string>& params);
        static drogon::HttpResponsePtr executeLuaTable(const luabridge::LuaRef& handler);
        static drogon::HttpResponsePtr executeLuaFunction(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params);

        static void sendJsonResponse(const Json::Value& json, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
        static void sendErrorResponse(const std::string& message, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

        static size_t countPathParameters(const std::string &path);

        static drogon::HttpResponsePtr executeRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler,
             const drogon::HttpRequestPtr& req, const std::vector<std::string>& params);

        static drogon::HttpResponsePtr executeMiddlewareChain(const LuaMiddlewareManager::MiddlewareChain& chain, std::size_t index, LuaRequest& req,
             LuaResponse& res, const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& httpReq, const std::vector<std::string>& params);

        static int luaRegister(lua_State* L, drogon::HttpMethod method, const char* methodName);
};
