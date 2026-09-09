#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaRoutes.h>

#include <string>

using namespace drogon;

namespace {
    lua_State *createLuaState() {
        lua_State *L = luaL_newstate();

        if (!L)
            return nullptr;

        luaL_openlibs(L);

        return L;
    }

    bool runLua(lua_State *L, const std::string &code) {
        if (luaL_dostring(L, code.c_str()) != LUA_OK) {
            const char *error = lua_tostring(L, -1);

            if (error)
                LOG_ERROR << "Lua test failed: " << error;

            lua_pop(L, 1);
            return false;
        }

        return true;
    }

    luabridge::LuaRef getLuaGlobal(lua_State *L, const std::string &name) {
        return luabridge::getGlobal(L, name.c_str());
    }

    bool callLuaRoute(lua_State *L, lua_CFunction route, const std::string &path, const luabridge::LuaRef &handler) {
        lua_pushcfunction(L, route);
        lua_pushlstring(L, path.data(), path.size());
        handler.push(L);

        const int status = lua_pcall(L, 2, 0, 0);

        if (status != LUA_OK) {
            const char *error = lua_tostring(L, -1);

            if (error)
                LOG_ERROR << "Route registration failed: " << error;

            lua_pop(L, 1);
            return false;
        }

        return true;
    }
}

// ============================================================
// Async GET with zero path parameters
// ============================================================

DROGON_TEST(LuaRoutesGetAsync) {
    lua_State *L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                message = "hello from async route"
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK(callLuaRoute(L, LuaRoutes::luaGetAsync, "/unit/lua/async", handler));
}
