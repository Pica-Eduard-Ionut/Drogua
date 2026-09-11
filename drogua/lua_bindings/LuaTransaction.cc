#include "LuaTransaction.h"
#include "LuaResult.h"
#include "LuaAsyncContextRegistry.h"

#include <drogon/drogon.h>

#include <stdexcept>
#include <utility>

void bindLuaParameters(drogon::orm::internal::SqlBinder& binder, const luabridge::LuaRef& params) {
    if (!params.isTable()) {
        throw std::runtime_error("Database query parameters must be a Lua table");
    }

    const auto length = params.length();
    for (int i = 1; i <= length; ++i) {
        luabridge::LuaRef value(params[i]);

        if (value.isNil()) {
            binder << nullptr;
        }

        else if (value.isBool()) {
            binder << value.cast<bool>().value();
        }

        else if (value.isNumber()) {
            /*
             * Lua 5.4 has two numeric types:
             *  - LUA_TINTEGER
             *  - LUA_TNUMBER
             *
             * LuaBridge3 exposes isNumber(), but not isInteger().
             * casting allows to use integers when the Lua value is actually an integer.
             */
            lua_State* L = value.state();

            value.push(L);

            if (lua_isinteger(L, -1)) {
                lua_Integer integerValue = lua_tointeger(L, -1);
                lua_pop(L, 1);

                binder << static_cast<int64_t>(integerValue);
            }
            
            else {
                lua_Number numberValue = lua_tonumber(L, -1);
                lua_pop(L, 1);

                binder << static_cast<double>(numberValue);
            }
        }

        else if (value.isString()) {
            binder << value.cast<std::string>().value();
        }

        else {
            throw std::runtime_error("Unsupported database parameter at index " + std::to_string(i));
        }
    }
}

LuaTransaction::LuaTransaction(std::shared_ptr<drogon::orm::Transaction> transaction)
    : transaction_(std::move(transaction)) {
    if (!transaction_) {
        throw std::runtime_error("Failed to create database transaction");
    }
}

LuaTransaction::~LuaTransaction() {
    if (transaction_ && !finished_) {
        try {
            /*
             * Drogon commits when Transaction is destroyed.
             * Rollback explicitly so an abandoned Lua transaction
             * can never be committed accidentally.
             */
            transaction_->rollback();
        }

        catch (...) {
            // Never throw from a destructor.
        }

        transaction_.reset();
        finished_ = true;
    }
}

bool LuaTransaction::valid() const {
    return transaction_ && !finished_;
}

std::shared_ptr<LuaResult> LuaTransaction::query(const std::string& sql) {
    if (!valid()) {
        throw std::runtime_error("Database transaction is no longer active");
    }

    try {
        return std::make_shared<LuaResult>(transaction_->execSqlSync(sql));
    }

    catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error(
            "Transaction database error: " + std::string(e.base().what()));
    }

    catch (const std::exception& e) {
        throw std::runtime_error(
            "Transaction database error: " + std::string(e.what()));
    }
}

std::shared_ptr<LuaResult> LuaTransaction::query(const std::string& sql, const luabridge::LuaRef& params) {
    if (!valid()) {
        throw std::runtime_error("Database transaction is no longer active");
    }

    if (!params.isTable()) {
        throw std::runtime_error("Database query parameters must be a Lua table");
    }

    try {
        auto binder = (*transaction_) << sql;
        bindLuaParameters(binder, params);
        binder << drogon::orm::Mode::Blocking;

        drogon::orm::Result result(nullptr);
        binder >> [&result](const drogon::orm::Result& r) {
            result = r;
        };
        binder.exec();

        return std::make_shared<LuaResult>(std::move(result));
    }


    catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error(
            "Transaction database error: " + std::string(e.base().what()));
    }

    catch (const std::exception& e) {
        throw std::runtime_error(
            "Transaction database error: " + std::string(e.what()));
    }
}

std::shared_ptr<LuaResult> LuaTransaction::queryLua(const std::string& sql, const luabridge::LuaRef& params) {
    if (params.isNil()) {
        return query(sql);
    }

    return query(sql, params);
}

std::size_t LuaTransaction::executeAffected(const std::string& sql) {
    return query(sql)->affectedRows();
}

unsigned long long LuaTransaction::lastInsertId(const std::string& sql) {
    return query(sql)->insertId();
}

void LuaTransaction::commit() {
    if (!transaction_ || finished_) {
        throw std::runtime_error("Database transaction is no longer active");
    }

    /*
     * Drogon commits when TransactionImpl is destroyed.
     *
     * reset() destroys the Transaction object if this is the last
     * shared_ptr, causing TransactionImpl to queue COMMIT on the
     * database connection's event loop.
     */
    finished_ = true;
    transaction_.reset();
}

void LuaTransaction::rollback() {
    if (!transaction_ || finished_) {
        return;
    }

    try {
        transaction_->rollback();
    }

    catch (const std::exception& e) {
        finished_ = true;
        transaction_.reset();

        throw std::runtime_error("Transaction rollback failed: " + std::string(e.what()));
    }

    finished_ = true;
    transaction_.reset();
}

void LuaTransaction::queryAsync(const std::string& sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    if (!valid()) {
        throw std::runtime_error("Database transaction is no longer active");
    }

    if (!callback) {
        throw std::runtime_error("Lua transaction async query callback is empty");
    }

    if (!errorCallback) {
        throw std::runtime_error("Lua transaction async error callback is empty");
    }

    transaction_->execSqlAsync(
        sql,
        [callback](const drogon::orm::Result& result) {
            callback(std::make_shared<LuaResult>(result));
        },
        [errorCallback](const drogon::orm::DrogonDbException& e) {
            errorCallback(e.base().what());
        });
}

void LuaTransaction::queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    if (!valid()) {
        throw std::runtime_error("Database transaction is no longer active");
    }

    if (!params.isTable()) {
        throw std::runtime_error("Database query parameters must be a Lua table");
    }

    if (!callback) {
        throw std::runtime_error("Lua transaction async query callback is empty");
    }

    if (!errorCallback) {
        throw std::runtime_error("Lua transaction async error callback is empty");
    }

    auto binder = (*transaction_) << sql;

    bindLuaParameters(binder, params);

    binder >> [callback](const drogon::orm::Result& result) {
        callback(std::make_shared<LuaResult>(result));
    };

    binder >> [errorCallback](const drogon::orm::DrogonDbException& e) {
        errorCallback(e.base().what());
    };

    binder.exec();
}

int LuaTransaction::queryAsyncLua(lua_State* L) {
    if (!L) {
        return luaL_error(L, "Transaction queryAsync received null Lua state");
    }

    if (!lua_isuserdata(L, 1)) {
        return luaL_error(L, "Transaction queryAsync expected a DatabaseTransaction");
    }

    auto transaction = luabridge::get<LuaTransaction*>(L, 1);

    if (!transaction) {
        return luaL_error(L, "Invalid DatabaseTransaction: %s", transaction.message().c_str());
    }

    LuaTransaction* tx = transaction.value();

    if (!tx) {
        return luaL_error(L, "DatabaseTransaction is null");
    }

    const char* sql = luaL_checkstring(L, 2);
    const int argc = lua_gettop(L);

    if (argc < 2 || argc > 3) {
        return luaL_error(L, "Transaction queryAsync expects sql and optional parameters");
    }

    auto context = LuaAsyncContextRegistry::get(L);

    if (!context) {
        return luaL_error(L, "Transaction queryAsync must be called from an async route");
    }

    if (!context->coroutine) {
        return luaL_error(L, "Transaction queryAsync has no active coroutine");
    }

    try {
        if (argc == 3) {
            if (!lua_istable(L, 3)) {
                return luaL_error(L, "Transaction queryAsync parameters must be a table");
            }

            auto paramsResult = luabridge::Stack<luabridge::LuaRef>::get(L, 3);

            if (!paramsResult) {
                return luaL_error(L, "Invalid query parameters: %s", paramsResult.message().c_str());
            }

            auto params = paramsResult.value();

            tx->queryAsync(
                sql,
                params,
                [context](std::shared_ptr<LuaResult> result) {
                    context->asyncResult = std::move(result);
                    context->asyncError.clear();

                    if (context->resume) {
                        context->resume();
                    }
                },
                [context](const std::string& error) {
                    context->asyncResult.reset();
                    context->asyncError = error;

                    if (context->resume) {
                        context->resume();
                    }
                });
        }
        else {
            tx->queryAsync(
                sql,
                [context](std::shared_ptr<LuaResult> result) {
                    context->asyncResult = std::move(result);
                    context->asyncError.clear();

                    if (context->resume) {
                        context->resume();
                    }
                },
                [context](const std::string& error) {
                    context->asyncResult.reset();
                    context->asyncError = error;

                    if (context->resume) {
                        context->resume();
                    }
                });
        }
    }
    catch (const std::exception& e) {
        return luaL_error(L, "Async transaction query failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaTransaction::queryAsyncContinuation);
}

int LuaTransaction::queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;

    if (!L) {
        return 0;
    }

    auto context = LuaAsyncContextRegistry::get(L);

    if (!context) {
        return luaL_error(L, "Transaction queryAsync continuation has no async context");
    }

    if (status != LUA_YIELD) {
        return luaL_error(L, "Transaction queryAsync continuation resumed with unexpected status");
    }

    if (!context->asyncError.empty()) {
        const std::string error = context->asyncError;
        context->asyncError.clear();

        return luaL_error(L, "Async transaction query failed: %s", error.c_str());
    }

    if (!context->asyncResult) {
        return luaL_error(L, "Async transaction query completed without a result");
    }

    auto result = context->asyncResult;
    context->asyncResult.reset();

    auto pushResult = luabridge::Stack<std::shared_ptr<LuaResult>>::push(L, result);

    if (!pushResult) {
        return luaL_error(L, "Failed to push async transaction result: %s", pushResult.message().c_str());
    }

    return 1;
}
