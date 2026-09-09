#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include <lua_bindings/LuaDatabase.h>
#include <lua_bindings/LuaResult.h>
#include <lua_bindings/LuaRow.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
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
        db->execute("DELETE FROM sqlite_sequence WHERE name = 'users'");
        db->execute("INSERT INTO users (name) VALUES ('Alice')");
        db->execute("INSERT INTO users (name) VALUES ('Bob')");
    }

    /*
    * Wait for an async database operation to finish.
    *
    * Returns true if either callback was invoked before
    * the timeout.
    */
    bool waitForCompletion(std::mutex& mutex, std::condition_variable& cv, bool& completed, bool& failed) {
        std::unique_lock<std::mutex> lock(mutex);

        return cv.wait_for(lock, std::chrono::seconds(5), [&]() {
            return completed || failed;
        });
    }
}

// ---------------------------------------------------------
// LuaDatabase Async Query
// ---------------------------------------------------------

DROGON_TEST(LuaDatabaseAsyncQuery) {
    setupUsersTable();
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    std::mutex mutex;
    std::condition_variable cv;

    bool completed = false;
    bool failed = false;

    db->queryAsync(
        R"(
            SELECT id, name
            FROM users
            ORDER BY id
        )",
        [&](std::shared_ptr<LuaResult> result) {
            {
                std::lock_guard<std::mutex> lock(mutex);
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
            }

            cv.notify_one();
        },
        [&](const std::string& error) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                LOG_ERROR << "Async database query failed: " << error;
            }

            cv.notify_one();
        });

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(completed);
    CHECK(!failed);
}

// ---------------------------------------------------------
// Empty Result
// ---------------------------------------------------------

DROGON_TEST(LuaDatabaseAsyncQueryEmptyResult) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    std::mutex mutex;
    std::condition_variable cv;

    bool completed = false;
    bool failed = false;

    db->queryAsync(
        R"(
            SELECT id, name
            FROM users
            WHERE id = 999999
        )",
        [&](std::shared_ptr<LuaResult> result) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                completed = true;

                REQUIRE(result != nullptr);

                CHECK(result->size() == 0);
                CHECK(result->count() == 0);
                /*
                 * Empty Drogon results do not expose column
                 * metadata through LuaResult.
                 */
                CHECK(result->columns() == 0);
            }

            cv.notify_one();
        },
        [&](const std::string& error) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                LOG_ERROR << "Async empty-result query failed: " << error;
            }

            cv.notify_one();
        });

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(completed);
    CHECK(!failed);
}

// ---------------------------------------------------------
// Query Error
// ---------------------------------------------------------
DROGON_TEST(LuaDatabaseAsyncQueryError) {
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    std::mutex mutex;
    std::condition_variable cv;

    bool completed = false;
    bool failed = false;

    std::string errorMessage;

    db->queryAsync(
        "SELECT * FROM table_that_does_not_exist",
        [&](std::shared_ptr<LuaResult> result) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                completed = true;
                /*
                 * The success callback should not be called
                 * for an invalid query.
                 */
                CHECK(result == nullptr);
            }

            cv.notify_one();
        },
        [&](const std::string& error) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                failed = true;
                errorMessage = error;

                LOG_INFO << "Expected async database error: " << error;
            }

            cv.notify_one();
        });

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(!completed);
    CHECK(failed);
    CHECK(!errorMessage.empty());
}
