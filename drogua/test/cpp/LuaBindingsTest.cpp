#include <drogon/drogon_test.h>
#include <lua.hpp>
#include <lua_bindings/LuaBindings.h>

namespace {
    lua_State* createLuaState() {
        lua_State* L = luaL_newstate();
        if (L) {
            luaL_openlibs(L);
            registerDrogua(L);
        }
        return L;
    }

    bool runLua(lua_State* L, const char* code) {
        if (luaL_dostring(L, code) != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            if (error) { LOG_ERROR << "Lua test failed: " << error; }

            lua_pop(L, 1);
            return false;
        }

        return true;
    }
}

DROGON_TEST(LuaBindings) {
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    registerDrogua(L);

    CHECK(runLua(L, R"(
        assert(Drogua ~= nil)
        assert(Drogua.print ~= nil)
        assert(Drogua.app ~= nil)
        assert(Drogua.Routes ~= nil)
        assert(Drogua.Request ~= nil)

        Drogua.print("Hello from Lua")
    )"));

    lua_close(L);
}

DROGON_TEST(LuaBindingsPrint) {
    lua_State* L = createLuaState();
    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        Drogua.print("Hello from Lua")
    )"));

    lua_close(L);
}