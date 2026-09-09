#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

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

        db->execute(
            "INSERT INTO users (name) VALUES ('Bob')"
        );
    }
}

// ---------------------------------------------------------
// LuaDatabase Async
// ---------------------------------------------------------

DROGON_TEST(LuaDatabaseAsyncQuery) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    bool completed = false;
    bool failed = false;

    db->queryAsync(
        R"(
            SELECT id, name
            FROM users
            ORDER BY id
        )",

        [&](std::shared_ptr<LuaResult> result) {
            completed = true;

            REQUIRE(result != nullptr);

            CHECK(result->size() == 2);
            CHECK(result->count() == 2);
            CHECK(result->columns() == 2);

            CHECK(result->columnName(0) == "id");
            CHECK(result->columnName(1) == "name");

            auto alice = result->row(0);

            REQUIRE(alice != nullptr);

            CHECK(alice->getString("id") == "1");
            CHECK(alice->getString("name") == "Alice");

            auto bob = result->row(1);

            REQUIRE(bob != nullptr);

            CHECK(bob->getString("id") == "2");
            CHECK(bob->getString("name") == "Bob");
        },

        [&](const std::string& error) {
            failed = true;

            LOG_ERROR << "Async database query failed: "
                      << error;
        }
    );

    CHECK(!completed);
    CHECK(!failed);

    app().getLoop()->runAfter(
        0.1,
        [&]() {
            CHECK(completed);
            CHECK(!failed);
        }
    );
}