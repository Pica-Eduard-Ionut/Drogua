#include <drogon/drogon.h>

// lua depends
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

// include libs for wrapper functions
#include <lua_bindings/LuaRoutes.h>
#include <lua_bindings/LuaRequest.h>

// temp
#include <iostream>
#include <string>

using namespace drogon;

// C++ function we want to expose to Lua
void draguaPrint(const std::string& message) { std::cout << "[Drogua] " << message << std::endl; }

// Config loading methods
void loadJsonConfig(const std::string& file) {
    app().loadConfigFile("./" + file + ".json");
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
    .beginNamespace("Drogua")
        .addFunction("print", &draguaPrint)
        .addFunction("loadJsonConfig", &loadJsonConfig)

        .beginNamespace("Routes")
            .addFunction("get", &LuaRoutes::get)
            .addFunction("post", &LuaRoutes::post)
            .addFunction("put", &LuaRoutes::put)
            .addFunction("delete", &LuaRoutes::del)
            .addFunction("patch", &LuaRoutes::patch)
        .endNamespace()

        .beginClass<LuaRequest>("Request")
            // Request metadata
            .addFunction("method", &LuaRequest::method)
            .addFunction("path", &LuaRequest::path)
            .addFunction("secure", &LuaRequest::secure)
            .addFunction("ip", &LuaRequest::ip)
            // Query parameters
            .addFunction("query", &LuaRequest::query)
            .addFunction("queryParams", &LuaRequest::queryParams)
            // Headers
            .addFunction("header", &LuaRequest::header)
            .addFunction("headers", &LuaRequest::headers)
            // Cookies
            .addFunction("cookie", &LuaRequest::cookie)
            .addFunction("cookies", &LuaRequest::cookies)
            // Body
            .addFunction("body", &LuaRequest::body)
            // JSON
            .addFunction("json", &LuaRequest::json)
        .endClass()
        
    .endNamespace();

    // Run a Lua script
    if (luaL_dofile(L, "app.lua") != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }

    //Set HTTP listener address and port
    app().addListener("0.0.0.0", 5555);

    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");

    //Run HTTP framework,the method will block in the internal event loop
    app().run();

    return 0;
}
