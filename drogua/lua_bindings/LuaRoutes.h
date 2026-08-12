#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

class LuaRoutes {
public:
    static void get(const std::string& path, const luabridge::LuaRef& handler);
    static void post(const std::string& path, const luabridge::LuaRef& handler);
    static void put(const std::string& path, const luabridge::LuaRef& handler);
    static void del(const std::string& path, const luabridge::LuaRef& handler);
    static void patch(const std::string& path, const luabridge::LuaRef& handler);
    static Json::Value luaTableToJson(const luabridge::LuaRef& table);

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

    static Json::Value executeHandler(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr &req, const std::vector<std::string>& params);
    static Json::Value executeLuaTable(const luabridge::LuaRef& handler);
    static Json::Value executeLuaFunction(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req, const std::vector<std::string> &params);

    static void sendJsonResponse(const Json::Value& json, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void sendErrorResponse(const std::string& message, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static size_t countPathParameters(const std::string &path);
};
