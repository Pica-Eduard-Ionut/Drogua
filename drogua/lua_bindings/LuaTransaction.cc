#include "LuaTransaction.h"
#include "LuaResult.h"

#include <drogon/drogon.h>

#include <stdexcept>
#include <utility>

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
                binder << value.cast<double>().value();
            }

            else if (value.isString()) {
                binder << value.cast<std::string>().value();
            }

            else {
                throw std::runtime_error(
                    "Unsupported database parameter at index " + std::to_string(i));
            }
        }

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
