#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaDatabase.h>
#include <lua_bindings/LuaTransaction.h>
#include <lua_bindings/LuaResult.h>
#include <lua_bindings/LuaRow.h>

#include <memory>
#include <string>
#include <cmath>

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

        db->execute(
            "INSERT INTO users (name) VALUES ('Bob')"
        );
    }

    lua_State* createLuaState() {
        lua_State* L = luaL_newstate();

        if (L)
            luaL_openlibs(L);

        return L;
    }
} // namespace

// ---------------------------------------------------------
// Construction
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionBasic) {
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    auto tx = db->begin();

    REQUIRE(tx != nullptr);
    CHECK(tx->valid());

    tx->rollback();

    CHECK(!tx->valid());
}

// ---------------------------------------------------------
// Parameterized query
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionParameterizedQuery) {
    setupUsersTable();

    auto db = database();
    auto tx = db->begin();

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    luabridge::LuaRef params = luabridge::newTable(L);
    params[1] = "Alice";

    auto result = tx->query(
        R"(
            SELECT id, name
            FROM users
            WHERE name = ?
            ORDER BY id
        )",
        params);

    REQUIRE(result != nullptr);
    CHECK(result->count() == 1);

    auto row = result->row(0);

    REQUIRE(row != nullptr);

    CHECK(row->getString("id") == "1");
    CHECK(row->getString("name") == "Alice");

    tx->commit();

    CHECK(!tx->valid());
}

// ---------------------------------------------------------
// Integer / double parameter binding
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionNumericParameters) {
    auto db = database();

    db->execute(R"(
        CREATE TABLE IF NOT EXISTS numeric_test (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            integer_value INTEGER,
            double_value REAL
        )
    )");

    db->execute("DELETE FROM numeric_test");

    auto tx = db->begin();

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    luabridge::LuaRef params = luabridge::newTable(L);
    params[1] = static_cast<lua_Integer>(42);
    params[2] = static_cast<lua_Number>(3.14);

    auto result = tx->query(
        R"(
            INSERT INTO numeric_test
                (integer_value, double_value)
            VALUES (?, ?)
        )",
        params);

    REQUIRE(result != nullptr);
    CHECK(result->affectedRows() == 1);

    tx->commit();

    auto check = db->query(R"(
        SELECT integer_value, double_value
        FROM numeric_test
        ORDER BY id
    )");

    REQUIRE(check != nullptr);
    REQUIRE(check->count() == 1);

    auto row = check->row(0);

    REQUIRE(row != nullptr);

    CHECK(row->getString("integer_value") == "42");
    CHECK(std::abs(std::stod(row->getString("double_value")) - 3.14) < 1e-9);
}

// ---------------------------------------------------------
// Commit
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionCommit) {
    setupUsersTable();

    auto db = database();
    auto tx = db->begin();

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    luabridge::LuaRef params = luabridge::newTable(L);
    params[1] = "Charlie";

    auto result = tx->query(
        "INSERT INTO users (name) VALUES (?)",
        params);

    REQUIRE(result != nullptr);
    CHECK(result->affectedRows() == 1);

    tx->commit();

    CHECK(!tx->valid());

    auto users = db->query(R"(
        SELECT id, name
        FROM users
        WHERE name = 'Charlie'
    )");

    REQUIRE(users != nullptr);
    CHECK(users->count() == 1);

    auto row = users->row(0);

    REQUIRE(row != nullptr);
    CHECK(row->getString("name") == "Charlie");
}

// ---------------------------------------------------------
// Rollback
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionRollback) {
    setupUsersTable();

    auto db = database();
    auto tx = db->begin();

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    luabridge::LuaRef params = luabridge::newTable(L);
    params[1] = "ShouldRollback";

    auto result = tx->query(
        "INSERT INTO users (name) VALUES (?)",
        params);

    REQUIRE(result != nullptr);
    CHECK(result->affectedRows() == 1);

    tx->rollback();

    CHECK(!tx->valid());

    auto users = db->query(R"(
        SELECT id, name
        FROM users
        WHERE name = 'ShouldRollback'
    )");

    REQUIRE(users != nullptr);
    CHECK(users->count() == 0);
}

// ---------------------------------------------------------
// Invalid after commit / rollback
// ---------------------------------------------------------
DROGON_TEST(LuaTransactionInvalidAfterFinish) {
    setupUsersTable();

    auto db = database();

    // -----------------------------------------------------
    // After commit
    // -----------------------------------------------------
    auto committed = db->begin();

    REQUIRE(committed != nullptr);
    REQUIRE(committed->valid());

    committed->commit();

    CHECK(!committed->valid());

    CHECK_THROWS_AS(committed->query("SELECT id FROM users"), std::runtime_error);

    // -----------------------------------------------------
    // After rollback
    // -----------------------------------------------------
    auto rolledBack = db->begin();

    REQUIRE(rolledBack != nullptr);
    REQUIRE(rolledBack->valid());

    rolledBack->rollback();

    CHECK(!rolledBack->valid());

    CHECK_THROWS_AS(rolledBack->query("SELECT id FROM users"), std::runtime_error);
}
