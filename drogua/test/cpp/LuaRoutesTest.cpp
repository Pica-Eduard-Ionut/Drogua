#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <lua_bindings/LuaRoutes.h>

#include <string>
#include <vector>

using namespace drogon;

namespace {
    std::vector<lua_State*>& luaStates() {
        static std::vector<lua_State*> states;
        return states;
    }

    lua_State* createLuaState() {
        lua_State* L = luaL_newstate();

        if (!L)
            return nullptr;

        luaL_openlibs(L);

        luaStates().push_back(L);

        return L;
    }

    bool runLua(lua_State* L, const std::string& code) {
        if (luaL_dostring(L, code.c_str()) != LUA_OK) {
            const char* error = lua_tostring(L, -1);

            if (error) {
                LOG_ERROR << "Lua test failed: " << error;
            }

            lua_pop(L, 1);
            return false;
        }

        return true;
    }

    luabridge::LuaRef getLuaGlobal(
        lua_State* L,
        const std::string& name) {

        return luabridge::getGlobal(
            L,
            name.c_str());
    }

} // namespace


// ============================================================
// GET
// ============================================================

DROGON_TEST(LuaRoutesGet) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                method = req:method()
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::get(
            "/unit/lua/get",
            handler
        )
    );
}


// ============================================================
// POST
// ============================================================

DROGON_TEST(LuaRoutesPost) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                method = req:method()
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::post(
            "/unit/lua/post",
            handler
        )
    );
}


// ============================================================
// PUT
// ============================================================

DROGON_TEST(LuaRoutesPut) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                method = req:method()
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::put(
            "/unit/lua/put",
            handler
        )
    );
}


// ============================================================
// DELETE
// ============================================================

DROGON_TEST(LuaRoutesDelete) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                method = req:method()
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::del(
            "/unit/lua/delete",
            handler
        )
    );
}


// ============================================================
// PATCH
// ============================================================

DROGON_TEST(LuaRoutesPatch) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {
                method = req:method()
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::patch(
            "/unit/lua/patch",
            handler
        )
    );
}


// ============================================================
// GET with one path parameter
// ============================================================

DROGON_TEST(LuaRoutesGetOnePathParameter) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req, id)
            return {
                id = id
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::get(
            "/unit/lua/users/{id}",
            handler
        )
    );
}


// ============================================================
// GET with multiple path parameters
// ============================================================

DROGON_TEST(LuaRoutesGetMultiplePathParameters) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req, user, post)
            return {
                user = user,
                post = post
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::get(
            "/unit/lua/users/{user}/posts/{post}",
            handler
        )
    );
}


// ============================================================
// GET with table handler
// ============================================================

DROGON_TEST(LuaRoutesGetTableHandler) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = {
            name = "Drogua",
            value = 42,
            active = true
        }
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isTable());

    CHECK_NOTHROW(
        LuaRoutes::get(
            "/unit/lua/table",
            handler
        )
    );
}


// ============================================================
// POST with table handler
// ============================================================

DROGON_TEST(LuaRoutesPostTableHandler) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = {
            message = "hello",
            value = 42
        }
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isTable());

    CHECK_NOTHROW(
        LuaRoutes::post(
            "/unit/lua/post-table",
            handler
        )
    );
}


// ============================================================
// PUT with path parameters
// ============================================================

DROGON_TEST(LuaRoutesPutPathParameters) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req, id)
            return {
                id = id
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::put(
            "/unit/lua/put/{id}",
            handler
        )
    );
}


// ============================================================
// DELETE with path parameters
// ============================================================

DROGON_TEST(LuaRoutesDeletePathParameters) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req, id)
            return {
                id = id
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::del(
            "/unit/lua/delete/{id}",
            handler
        )
    );
}


// ============================================================
// PATCH with path parameters
// ============================================================

DROGON_TEST(LuaRoutesPatchPathParameters) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req, id)
            return {
                id = id
            }
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::patch(
            "/unit/lua/patch/{id}",
            handler
        )
    );
}


// ============================================================
// Invalid handler type
// ============================================================

DROGON_TEST(LuaRoutesInvalidHandler) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = "invalid"
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(!handler.isFunction());
    REQUIRE(!handler.isTable());

    CHECK_THROWS(
        LuaRoutes::get(
            "/unit/lua/invalid-handler",
            handler
        )
    );
}


// ============================================================
// Empty path
// ============================================================

DROGON_TEST(LuaRoutesEmptyPath) {
    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    CHECK(runLua(L, R"(
        handler = function(req)
            return {}
        end
    )"));

    auto handler = getLuaGlobal(L, "handler");

    REQUIRE(handler.isFunction());

    CHECK_NOTHROW(
        LuaRoutes::get(
            "",
            handler
        )
    );
}