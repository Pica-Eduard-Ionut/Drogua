#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <lua.hpp>
#include <lua_bindings/LuaBindings.h>

#include <string>

using namespace drogon;

namespace {
    bool runLuaFile(lua_State* L, const std::string& path) {
        if (luaL_dofile(L, path.c_str()) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            if (error) { LOG_ERROR << "Lua test failed in " << path << ": " << error; }
            lua_pop(L, 1);
            return false;
        }
        return true;
    }
}

DROGON_TEST(LuaBindingsApp) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);
    CHECK(runLuaFile(L, "lua/test_app.lua"));
}

DROGON_TEST(LuaRoutesIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);
    CHECK(runLuaFile(L, "lua/test_routes.lua"));
    CHECK(runLuaFile(L, "lua/test_routes_http.lua"));
}

DROGON_TEST(LuaRequestIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLuaFile(L, "lua/test_requests.lua"));
    CHECK(runLuaFile(L, "lua/test_requests_http.lua"));
}

DROGON_TEST(LuaResponseIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLuaFile(L, "lua/test_response.lua"));
    CHECK(runLuaFile(L, "lua/test_response_http.lua"));
}

DROGON_TEST(LuaDatabaseIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLuaFile(L, "lua/test_database.lua"));
    CHECK(runLuaFile(L, "lua/test_database_http.lua"));
}

DROGON_TEST(LuaTransactionIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLuaFile(L, "lua/test_transaction.lua"));
    CHECK(runLuaFile(L, "lua/test_transaction_http.lua"));
}

DROGON_TEST(LuaMiddlewareIntegration) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLuaFile(L, "lua/test_middleware.lua"));
    CHECK(runLuaFile(L, "lua/test_middleware_http.lua"));
}