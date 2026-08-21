#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaDatabase.h>
#include <lua_bindings/LuaResult.h>
#include <lua_bindings/LuaRow.h>

#include <memory>
#include <string>

using namespace drogon;

namespace {
    constexpr const char* TEST_DB = "lua_test_db";

    std::shared_ptr<LuaDatabase> database() {
        return std::make_shared<LuaDatabase>(TEST_DB);
    }

    void setupUsersTable() {
        auto db = database();

        db->execute(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL
            )
        )");

        db->execute("DELETE FROM users");

        db->execute(
            "DELETE FROM sqlite_sequence WHERE name = 'users'"
        );

        db->execute(
            "INSERT INTO users (name) VALUES ('Alice')"
        );
    }
}

// ---------------------------------------------------------
// LuaDatabase
// ---------------------------------------------------------

DROGON_TEST(LuaDatabaseBasic)
{
    auto db = database();

    REQUIRE(db != nullptr);

    CHECK(db->name() == TEST_DB);
    CHECK(db->valid());
}

DROGON_TEST(LuaDatabaseExecute)
{
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    db->execute(R"(
        CREATE TABLE IF NOT EXISTS execute_test (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        )
    )");

    auto result = db->execute("INSERT INTO execute_test (name) VALUES ('Test')");

    REQUIRE(result != nullptr);

    CHECK(result->affectedRows() == 1);
    CHECK(result->insertId() > 0);
}

DROGON_TEST(LuaDatabaseInsert)
{
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    db->execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        )
    )");

    db->execute("DELETE FROM users");

    auto result = db->execute("INSERT INTO users (name) VALUES ('Alice')");

    REQUIRE(result != nullptr);

    CHECK(result->affectedRows() == 1);
    CHECK(result->insertId() > 0);
}

// ---------------------------------------------------------
// LuaResult
// ---------------------------------------------------------

DROGON_TEST(LuaResultQuery)
{
    setupUsersTable();

    auto db = database();
    auto result = db->query(R"(
        SELECT id, name
        FROM users
        ORDER BY id
    )");

    REQUIRE(result != nullptr);

    CHECK(result->size() == 1);
    CHECK(result->count() == 1);
    CHECK(result->columns() == 2);
    CHECK(result->columnName(0) == "id");
    CHECK(result->columnName(1) == "name");
}

DROGON_TEST(LuaRowAccess)
{
    setupUsersTable();

    auto db = database();
    auto result = db->query("SELECT id, name FROM users ORDER BY id");

    REQUIRE(result != nullptr);
    REQUIRE(result->count() == 1);

    auto row = result->row(0);

    REQUIRE(row != nullptr);

    CHECK(row->size() == 2);
    CHECK(row->columnName(0) == "id");
    CHECK(row->columnName(1) == "name");
    CHECK(!row->isNull("id"));
    CHECK(!row->isNull("name"));
    CHECK(row->getString("id") == "1");
    CHECK(row->getString("name") == "Alice");
    CHECK(row->toString() == "{id=1, name=Alice}");
}

DROGON_TEST(LuaRowIndexAccess)
{
    setupUsersTable();

    auto db = database();
    auto result = db->query("SELECT id, name FROM users ORDER BY id");

    REQUIRE(result != nullptr);
    REQUIRE(result->count() == 1);

    auto row = result->row(0);

    REQUIRE(row != nullptr);

    CHECK(!row->isNull(0));
    CHECK(!row->isNull(1));
    CHECK(row->getString(0) == "1");
    CHECK(row->getString(1) == "Alice");
}

DROGON_TEST(LuaRowNull)
{
    auto db = database();

    db->execute(R"(
        CREATE TABLE IF NOT EXISTS nullable_test (
            id INTEGER,
            value TEXT
        )
    )");

    db->execute("DELETE FROM nullable_test");
    db->execute("INSERT INTO nullable_test (id, value) VALUES (1, NULL)");

    auto result = db->query("SELECT id, value FROM nullable_test");

    REQUIRE(result != nullptr);
    REQUIRE(result->count() == 1);

    auto row = result->row(0);

    REQUIRE(row != nullptr);

    CHECK(!row->isNull("id"));
    CHECK(row->isNull("value"));
    CHECK(row->getString("id") == "1");
    CHECK(row->getString("value").empty());
}

// ---------------------------------------------------------
// LuaResult::pushTable
// ---------------------------------------------------------

DROGON_TEST(LuaResultPushTable)
{
    setupUsersTable();

    auto db = database();
    auto result = db->query(R"(
        SELECT id, name
        FROM users
        ORDER BY id
    )");

    REQUIRE(result != nullptr);
    REQUIRE(result->count() == 1);

    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);

    // Push the result onto the Lua stack.
    result->pushTable(L);

    REQUIRE(lua_istable(L, -1));

    // Stack:
    // [result table]

    // Get table[1].
    lua_rawgeti(L, -1, 1);

    REQUIRE(lua_istable(L, -1));

    // Stack:
    // [result table]
    // [row table]

    // Get table[1]["id"].
    lua_getfield(L, -1, "id");

    REQUIRE(lua_isstring(L, -1));
    CHECK(std::string(lua_tostring(L, -1)) == "1");

    lua_pop(L, 1);

    // Get table[1]["name"].
    lua_getfield(L, -1, "name");

    REQUIRE(lua_isstring(L, -1));
    CHECK(std::string(lua_tostring(L, -1)) == "Alice");

    lua_pop(L, 1);

    // Pop row table.
    lua_pop(L, 1);

    // Pop result table.
    lua_pop(L, 1);

    lua_close(L);
}