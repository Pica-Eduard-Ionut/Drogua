#include <drogon/drogon_test.h>
#include <lua.hpp>

#include <lua_bindings/LuaBindings.h>
#include <lua_bindings/LuaCoroutineManager.h>

namespace {
    lua_State *createLuaState() {
        lua_State *L = luaL_newstate();

        if (L) {
            luaL_openlibs(L);
            registerDrogua(L);
        }

        return L;
    }

    bool runLua(lua_State *L, const char *code) {
        if (luaL_dostring(L, code) != LUA_OK) {
            const char *error = lua_tostring(L, -1);

            if (error)
                LOG_ERROR << "Lua test failed: " << error;

            lua_pop(L, 1);
            return false;
        }

        return true;
    }
}

DROGON_TEST(LuaCoroutineManagerCreate) {
    lua_State *L = createLuaState();
    REQUIRE(L != nullptr);

    {
        auto coroutine = LuaCoroutineManager::create(L);

        REQUIRE(coroutine != nullptr);
        REQUIRE(coroutine->thread != nullptr);
        REQUIRE(coroutine->owner == L);
    }
}

DROGON_TEST(LuaCoroutineManagerNormalFunction) {
    lua_State *L = createLuaState();
    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        function testFunction()
            return 42
        end
    )"));

    auto function = luabridge::getGlobal(L, "testFunction");
    REQUIRE(function.isFunction());

    auto coroutine = LuaCoroutineManager::create(L);
    LuaCoroutineManager::pushFunction(coroutine, function);
    auto result = LuaCoroutineManager::resume(coroutine);

    CHECK(result.status == LuaCoroutineManager::Status::Finished);
    CHECK(result.nresults == 1);

    lua_State *co = LuaCoroutineManager::state(coroutine);

    CHECK(lua_isinteger(co, -1));
    CHECK(lua_tointeger(co, -1) == 42);
}

DROGON_TEST(LuaCoroutineManagerYieldAndResume) {
    lua_State *L = createLuaState();
    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        function testFunction()
            local value = coroutine.yield("paused")

            return value + 10
        end
    )"));

    auto function = luabridge::getGlobal(L, "testFunction");
    REQUIRE(function.isFunction());

    auto coroutine = LuaCoroutineManager::create(L);
    LuaCoroutineManager::pushFunction(coroutine, function);

    /*
     * First resume:
     *
     *     testFunction()
     *          |
     *          v
     *     coroutine.yield("paused")
     */
    auto first = LuaCoroutineManager::resume(coroutine);

    CHECK(first.status == LuaCoroutineManager::Status::Yielded);
    CHECK(first.nresults == 1);

    lua_State *co = LuaCoroutineManager::state(coroutine);

    CHECK(lua_isstring(co, -1));
    CHECK(std::string(lua_tostring(co, -1)) == "paused");

    /*
     * Remove the yielded value.
     */
    lua_pop(co, 1);

    /*
     * Resume the coroutine with:
     *
     *     value = 32
     *
     * The Lua function should return:
     *
     *     32 + 10 = 42
     */
    lua_pushinteger(co, 32);

    auto second = LuaCoroutineManager::resume(coroutine, 1);

    CHECK(second.status == LuaCoroutineManager::Status::Finished);
    CHECK(second.nresults == 1);

    CHECK(lua_isinteger(co, -1));
    CHECK(lua_tointeger(co, -1) == 42);
}

DROGON_TEST(LuaCoroutineManagerError) {
    lua_State *L = createLuaState();
    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        function testFunction()
            error("coroutine test error")
        end
    )"));

    auto function = luabridge::getGlobal(L, "testFunction");
    REQUIRE(function.isFunction());

    auto coroutine = LuaCoroutineManager::create(L);
    LuaCoroutineManager::pushFunction(coroutine, function);
    auto result = LuaCoroutineManager::resume(coroutine);

    CHECK(result.status == LuaCoroutineManager::Status::Error);
    CHECK(result.error.find("coroutine test error") != std::string::npos);
}
