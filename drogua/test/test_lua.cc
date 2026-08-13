#include <drogon/drogon_test.h>
// lua includes
#include <lua.hpp>
#include <lua_bindings/LuaBindings.h>
#include <string>

namespace {
    bool runLuaFile(lua_State* L, const std::string& path) {
        if (luaL_dofile(L, path.c_str()) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            if (error) { LOG_ERROR << "Lua test failed: " << error; }
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

    // run lua files
    CHECK(runLuaFile(L, "lua/test_app.lua"));

    lua_close(L);
}