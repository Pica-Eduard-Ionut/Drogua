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
#include <atomic>
#include <mutex>
#include <condition_variable>

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

    lua_State* createLuaState() {
        lua_State* L = luaL_newstate();

        if (L) {
            luaL_openlibs(L);
        }

        return L;
    }

    // Wait for an async Drogon operation without blocking the event-loop thread itself. These tests assume the Drogon event loop is running on another thread, as in the test main below.
    class AsyncWait {
    public:
        void complete() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                done_ = true;
            }

            condition_.notify_one();
        }

        void wait() {
            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this] {
                return done_;
            });
        }

    private:
        std::mutex mutex_;
        std::condition_variable condition_;
        bool done_{false};
    };
}

// Async transaction begin
DROGON_TEST(LuaTransactionAsyncBegin) {
    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait wait;

    std::shared_ptr<LuaTransaction> transaction;
    std::string error;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> tx) {
            transaction = std::move(tx);
            wait.complete();
        },
        [&](const std::string& err) {
            error = err;
            wait.complete();
        });

    wait.wait();

    CHECK(error.empty());

    REQUIRE(transaction != nullptr);
    CHECK(transaction->valid());

    transaction->rollback();

    CHECK(!transaction->valid());
}

// Async parameterized query
DROGON_TEST(LuaTransactionAsyncParameterizedQuery) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction) {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error) {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    lua_State* L = createLuaState();

    REQUIRE(L != nullptr);

    luabridge::LuaRef params = luabridge::newTable(L);
    params[1] = "Alice";

    AsyncWait queryWait;

    std::shared_ptr<LuaResult> result;
    std::string queryError;

    tx->queryAsync(
        R"(
            SELECT id, name
            FROM users
            WHERE name = ?
            ORDER BY id
        )",
        params,
        [&](std::shared_ptr<LuaResult> queryResult) {
            result = std::move(queryResult);
            queryWait.complete();
        },
        [&](const std::string& error) {
            queryError = error;
            queryWait.complete();
        });

    queryWait.wait();

    CHECK(queryError.empty());

    REQUIRE(result != nullptr);
    CHECK(result->count() == 1);

    auto row = result->row(0);

    REQUIRE(row != nullptr);

    CHECK(row->getString("id") == "1");
    CHECK(row->getString("name") == "Alice");

    tx->commit();

    CHECK(!tx->valid());
}

// Async multiple queries in same transaction
DROGON_TEST(LuaTransactionAsyncMultipleQueries) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction) {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error) {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    AsyncWait firstQueryWait;

    std::shared_ptr<LuaResult> users;
    std::string firstError;

    tx->queryAsync(
        R"(
            SELECT id, name
            FROM users
            ORDER BY id
        )",
        [&](std::shared_ptr<LuaResult> result) {
            users = std::move(result);
            firstQueryWait.complete();
        },
        [&](const std::string& error) {
            firstError = error;
            firstQueryWait.complete();
        });

    firstQueryWait.wait();

    CHECK(firstError.empty());

    REQUIRE(users != nullptr);
    CHECK(users->count() == 2);

    AsyncWait secondQueryWait;

    std::shared_ptr<LuaResult> count;
    std::string secondError;

    tx->queryAsync(
        R"(
            SELECT COUNT(*) AS total
            FROM users
        )",
        [&](std::shared_ptr<LuaResult> result) {
            count = std::move(result);
            secondQueryWait.complete();
        },
        [&](const std::string& error) {
            secondError = error;
            secondQueryWait.complete();
        });

    secondQueryWait.wait();

    CHECK(secondError.empty());

    REQUIRE(count != nullptr);
    CHECK(count->count() == 1);

    auto row = count->row(0);

    REQUIRE(row != nullptr);

    CHECK(row->getString("total") == "2");

    tx->commit();

    CHECK(!tx->valid());
}

// Async commit
DROGON_TEST(LuaTransactionAsyncCommit) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction) {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error) {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    AsyncWait queryWait;

    std::shared_ptr<LuaResult> result;
    std::string queryError;

    tx->queryAsync(
        "INSERT INTO users (name) VALUES ('AsyncCommit')",
        [&](std::shared_ptr<LuaResult> queryResult) {
            result = std::move(queryResult);
            queryWait.complete();
        },
        [&](const std::string& error) {
            queryError = error;
            queryWait.complete();
        });

    queryWait.wait();

    CHECK(queryError.empty());

    REQUIRE(result != nullptr);
    CHECK(result->affectedRows() == 1);

    tx->commit();

    CHECK(!tx->valid());

    auto users = db->query(R"(
        SELECT id, name
        FROM users
        WHERE name = 'AsyncCommit'
    )");

    REQUIRE(users != nullptr);
    CHECK(users->count() == 1);

    auto row = users->row(0);

    REQUIRE(row != nullptr);
    CHECK(row->getString("name") == "AsyncCommit");
}

// Async rollback
DROGON_TEST(LuaTransactionAsyncRollback) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction) {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error) {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    AsyncWait queryWait;

    std::shared_ptr<LuaResult> result;
    std::string queryError;

    tx->queryAsync(
        "INSERT INTO users (name) VALUES ('AsyncRollback')",
        [&](std::shared_ptr<LuaResult> queryResult) {
            result = std::move(queryResult);
            queryWait.complete();
        },
        [&](const std::string& error) {
            queryError = error;
            queryWait.complete();
        });

    queryWait.wait();

    CHECK(queryError.empty());

    REQUIRE(result != nullptr);
    CHECK(result->affectedRows() == 1);

    tx->rollback();

    CHECK(!tx->valid());

    auto users = db->query(R"(
        SELECT id, name
        FROM users
        WHERE name = 'AsyncRollback'
    )");

    REQUIRE(users != nullptr);
    CHECK(users->count() == 0);
}

// Invalid after async commit / rollback
DROGON_TEST(LuaTransactionAsyncInvalidAfterFinish) {
    setupUsersTable();

    auto db = database();

    // After commit
    AsyncWait commitBeginWait;

    std::shared_ptr<LuaTransaction> committed;
    std::string commitBeginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> tx) {
            committed = std::move(tx);
            commitBeginWait.complete();
        },
        [&](const std::string& error) {
            commitBeginError = error;
            commitBeginWait.complete();
        });

    commitBeginWait.wait();

    CHECK(commitBeginError.empty());

    REQUIRE(committed != nullptr);
    REQUIRE(committed->valid());

    committed->commit();

    CHECK(!committed->valid());

    CHECK_THROWS_AS(committed->query("SELECT id FROM users"), std::runtime_error);

    // After rollback
    AsyncWait rollbackBeginWait;

    std::shared_ptr<LuaTransaction> rolledBack;
    std::string rollbackBeginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> tx) {
            rolledBack = std::move(tx);
            rollbackBeginWait.complete();
        },
        [&](const std::string& error) {
            rollbackBeginError = error;
            rollbackBeginWait.complete();
        });

    rollbackBeginWait.wait();

    CHECK(rollbackBeginError.empty());

    REQUIRE(rolledBack != nullptr);
    REQUIRE(rolledBack->valid());

    rolledBack->rollback();

    CHECK(!rolledBack->valid());

    CHECK_THROWS_AS(rolledBack->query("SELECT id FROM users"), std::runtime_error);
}

DROGON_TEST(LuaTransactionAsyncQueryError)
{
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction)
        {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error)
        {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    AsyncWait queryWait;

    std::shared_ptr<LuaResult> result;
    std::string queryError;

    tx->queryAsync(
        "SELECT * FROM table_that_does_not_exist",
        [&](std::shared_ptr<LuaResult> queryResult)
        {
            result = std::move(queryResult);
            queryWait.complete();
        },
        [&](const std::string& error)
        {
            queryError = error;
            queryWait.complete();
        });

    queryWait.wait();

    CHECK(result == nullptr);
    CHECK(!queryError.empty());

    // The transaction should still be usable after
    // the failed statement, so explicitly roll it back.
    CHECK(tx->valid());

    tx->rollback();

    CHECK(!tx->valid());
}

DROGON_TEST(LuaTransactionAsyncRollbackAfterQueryError) {
    setupUsersTable();

    auto db = database();

    REQUIRE(db != nullptr);
    REQUIRE(db->valid());

    AsyncWait beginWait;

    std::shared_ptr<LuaTransaction> tx;
    std::string beginError;

    db->beginAsync(
        [&](std::shared_ptr<LuaTransaction> transaction) {
            tx = std::move(transaction);
            beginWait.complete();
        },
        [&](const std::string& error) {
            beginError = error;
            beginWait.complete();
        });

    beginWait.wait();

    CHECK(beginError.empty());

    REQUIRE(tx != nullptr);
    REQUIRE(tx->valid());

    // First query succeeds
    AsyncWait insertWait;

    std::shared_ptr<LuaResult> insertResult;
    std::string insertError;

    tx->queryAsync(
        "INSERT INTO users (name) VALUES ('ShouldRollback')",
        [&](std::shared_ptr<LuaResult> result) {
            insertResult = std::move(result);
            insertWait.complete();
        },
        [&](const std::string& error) {
            insertError = error;
            insertWait.complete();
        });

    insertWait.wait();

    CHECK(insertError.empty());

    REQUIRE(insertResult != nullptr);
    CHECK(insertResult->affectedRows() == 1);

    // Second query fails
    AsyncWait errorWait;

    std::shared_ptr<LuaResult> errorResult;
    std::string queryError;

    tx->queryAsync(
        "SELECT * FROM table_that_does_not_exist",
        [&](std::shared_ptr<LuaResult> result) {
            errorResult = std::move(result);
            errorWait.complete();
        },
        [&](const std::string& error) {
            queryError = error;
            errorWait.complete();
        });

    errorWait.wait();

    CHECK(errorResult == nullptr);
    CHECK(!queryError.empty());

    // Transaction must still be active.
    CHECK(tx->valid());

    // Roll back everything
    tx->rollback();

    CHECK(!tx->valid());

    // Verify successful INSERT was also rolled back
    auto users = db->query(R"(
        SELECT id, name
        FROM users
        WHERE name = 'ShouldRollback'
    )");

    REQUIRE(users != nullptr);
    CHECK(users->count() == 0);
}
