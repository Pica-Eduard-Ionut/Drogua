#include <drogon/drogon.h>

// lua depends
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

// temp
#include <iostream>
#include <string>

using namespace drogon;

// C++ function we want to expose to Lua
void draguaPrint(const std::string& message) { std::cout << "[Dragua] " << message << std::endl; }
// void draguaIntPrint(const int& message) { std::cout << "[Dragua INT] " << message << std::endl; }

// Config loading methods
void loadJsonConfig(const std::string& file) {
    app().loadConfigFile("./" + file + ".json");
}

// Routes
void registerLuaRoute(const std::string& path, const std::string& message) {
    app().registerHandler(path, [message](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) -> void {
                      Json::Value ret;
                      ret["message"] = message;
                      auto resp = HttpResponse::newHttpJsonResponse(ret);
                      callback(resp);
                  }
    );
}

// Convert a Lua table into a Json::Value object
Json::Value luaTableToJson(const luabridge::LuaRef& table) {
    Json::Value result;
    if (!table.isTable()) return result;

    lua_State* L = table.state();
    table.push(L);
    const int tableIndex = lua_gettop(L);
    // Push first key
    lua_pushnil(L);
    while (lua_next(L, tableIndex) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(L, -2);
            switch (lua_type(L, -1)) {
                case LUA_TSTRING:
                    result[key] = lua_tostring(L, -1);
                    break;

                case LUA_TNUMBER:
                    // Lua integer
                    if (lua_isinteger(L, -1)) result[key] = static_cast<Json::Int64>(lua_tointeger(L, -1));
                    // Lua floating-point number
                    else result[key] = lua_tonumber(L, -1);
                    break;

                case LUA_TBOOLEAN:
                    result[key] = lua_toboolean(L, -1) != 0;
                    break;

                case LUA_TNIL:
                    result[key] = Json::nullValue;
                    break;

                default:
                    std::cerr << "Unsupported Lua value for key '" << key << "'\n";
                    break;
            }
        }
        // Remove value, keep key for lua_next()
        lua_pop(L, 1);
    }
    // Remove the table
    lua_pop(L, 1);
    return result;
}


void registerRoute(const std::string& path, const luabridge::LuaRef& luaData) {
    Json::Value routeData = luaTableToJson(luaData);

    app().registerHandler(path, [routeData](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
        Json::Value ret = routeData;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
    );
}


int main() {
    // Lua setup
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state\n";
        return 1;
    }
    // Load standard Lua libraries (print, math, string, etc.)
    luaL_openlibs(L);

    // LuaBridge3 registration
    luabridge::getGlobalNamespace(L)
    .beginNamespace("Dragua")
    .addFunction("print", &draguaPrint)
    .addFunction("loadJsonConfig", &loadJsonConfig)
    .addFunction("addRoute", &registerLuaRoute)
    .addFunction("registerRoute", &registerRoute)
    // .addFunction("printInt", &draguaIntPrint)
    .endNamespace();

    // Run a Lua script
    if (luaL_dofile(L, "app.lua") != LUA_OK) {
        std::cerr << "Lua error: "
        << lua_tostring(L, -1)
        << std::endl;

        lua_pop(L, 1);
    }

    //Set HTTP listener address and port
    app().addListener("0.0.0.0", 5555);
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop

    app().registerHandler("/",
                          [](const HttpRequestPtr& req,
                             std::function<void (const HttpResponsePtr &)> &&callback) -> void
                             {
                                 Json::Value ret;
                                 ret["message"] = "Hello, World!";
                                 HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(ret);
                                 callback(resp);
                             });

    app().run();

    return 0;
}
