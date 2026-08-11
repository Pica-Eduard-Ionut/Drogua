#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <lua_bindings/LuaRequest.h>

class LuaRoutes {
public:
    static void get(const std::string& path, const luabridge::LuaRef& handler);
    static void post(const std::string& path, const luabridge::LuaRef& handler);
    static void put(const std::string& path, const luabridge::LuaRef& handler);
    static void del(const std::string& path, const luabridge::LuaRef& handler);
    static void patch(const std::string& path, const luabridge::LuaRef& handler);
    static Json::Value luaTableToJson(const luabridge::LuaRef& table);

private:
    static void registerRoute(const std::string& path, drogon::HttpMethod method, const luabridge::LuaRef& handler);

    static Json::Value executeHandler(const luabridge::LuaRef& handler);
    static Json::Value executeLuaFunction(const luabridge::LuaRef& handler);
    static Json::Value executeLuaTable(const luabridge::LuaRef& handler);

    static void sendJsonResponse(const Json::Value& json, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void sendErrorResponse(const std::string& message, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
