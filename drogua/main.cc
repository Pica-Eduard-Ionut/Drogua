#include <lua.hpp>

#include "lua_bindings/LuaBindings.h"
#include "lua_bindings/LuaMiddlewareManager.h"

#include <iostream>

int main() {
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state\n";
        return 1;
    }

    luaL_openlibs(L);
    registerDrogua(L);

    if (luaL_dofile(L, "app.lua") != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);

        LuaMiddlewareManager::instance().clear();

        lua_close(L);
        return 1;
    }

    LuaMiddlewareManager::instance().clear();

    lua_close(L);
    return 0;
}