#include "LuaTransaction.h"
#include "LuaResult.h"
#include "LuaAsyncContextRegistry.h"
#include "LuaAsyncContext.h"
#include "LuaDbUtils.h"

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
            // Rollback abandoned transactions to prevent accidental commits
            transaction_->rollback();
        } catch (...) {
            // Never throw from a destructor
        }
        transaction_.reset();
        finished_ = true;
    }
}

bool LuaTransaction::valid() const {
    return transaction_ && !finished_;
}

std::shared_ptr<LuaResult> LuaTransaction::query(const std::string& sql) {
    return executeQueryInternal(sql, nullptr);
}

std::shared_ptr<LuaResult> LuaTransaction::query(const std::string& sql, const luabridge::LuaRef& params) {
    if (!params.isTable()) throw std::runtime_error("Database query parameters must be a Lua table");  // ← Inlined
    return executeQueryInternal(sql, &params);
}

std::shared_ptr<LuaResult> LuaTransaction::queryLua(const std::string& sql, const luabridge::LuaRef& params) {
    // Delegates to existing overloads - no change needed
    return params.isNil() ? query(sql) : query(sql, params);
}

std::size_t LuaTransaction::executeAffected(const std::string& sql) {
    return query(sql)->affectedRows();
}

unsigned long long LuaTransaction::lastInsertId(const std::string& sql) {
    return query(sql)->insertId();
}

void LuaTransaction::commit() {
    ensureValid();
    // Drogon commits automatically when Transaction is destroyed
    finishTransaction();
}

void LuaTransaction::rollback() {
    if (!transaction_ || finished_)
        return;

    try {
        transaction_->rollback();
    }

    catch (const std::exception& e) {
        finishTransaction();
        throw std::runtime_error("Transaction rollback failed: " + std::string(e.what()));
    }

    finishTransaction();
}

void LuaTransaction::queryAsync(const std::string& sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    executeQueryAsyncInternal(sql, nullptr, std::move(callback), std::move(errorCallback));
}

void LuaTransaction::queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    if (!params.isTable()) throw std::runtime_error("Database query parameters must be a Lua table");
    executeQueryAsyncInternal(sql, &params, std::move(callback), std::move(errorCallback));
}

int LuaTransaction::queryAsyncLua(lua_State* L) {
    auto [tx, routeContext] = LuaDbUtils::extractAsyncLuaArgs<LuaTransaction>(L, "Transaction queryAsync");
    const char* sql = luaL_checkstring(L, 2);
    const int argc = lua_gettop(L);
    if (argc < 2 || argc > 3) 
        return luaL_error(L, "Transaction queryAsync expects sql and optional parameters");

    auto context = std::make_shared<LuaAsyncTransactionContext>();
    context->coroutine = routeContext->coroutine;
    context->callback = routeContext->callback;
    context->resume = routeContext->resume;
    context->request = routeContext->request;
    LuaAsyncContextRegistry::set<LuaAsyncTransactionContext>(L, context);

    auto successCb = [context](std::shared_ptr<LuaResult> result) {
        if (context->request && !context->request->connected()) return;
        context->asyncResult = std::move(result);
        context->asyncError.clear();
        if (context->resume) context->resume();
        else LOG_ERROR << "Async transaction query completed, but no resume handler installed";
    };

    auto errorCb = [context](const std::string& error) {
        if (context->request && !context->request->connected()) return;
        context->asyncResult.reset();
        context->asyncError = error;
        if (context->resume) context->resume();
        else LOG_ERROR << "Async transaction query failed, but no resume handler installed";
    };

    try {
        if (argc == 3) {
            if (!lua_istable(L, 3)) {
                LuaDbUtils::clearAsyncContext<LuaAsyncTransactionContext>(L); 
                return luaL_error(L, "Parameters must be a table"); 
            }

            auto paramsResult = luabridge::Stack<luabridge::LuaRef>::get(L, 3);
            if (!paramsResult) { 
                LuaDbUtils::clearAsyncContext<LuaAsyncTransactionContext>(L); 
                return luaL_error(L, "Invalid parameters: %s", paramsResult.message().c_str()); 
            }
            tx->executeQueryAsyncInternal(sql, &paramsResult.value(), successCb, errorCb);

        } else {
            tx->executeQueryAsyncInternal(sql, nullptr, successCb, errorCb);
        }

    } catch (const std::exception& e) {
        LuaDbUtils::clearAsyncContext<LuaAsyncTransactionContext>(L);
        return luaL_error(L, "Async transaction query failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaTransaction::queryAsyncContinuation);
}

int LuaTransaction::queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    return LuaDbUtils::handleQueryContinuation<LuaAsyncTransactionContext>(L, status, ctx, "transaction query");
}

// === Validation & Cleanup Helpers ===
void LuaTransaction::ensureValid() const {
    if (!valid()) throw std::runtime_error("Database transaction is no longer active");
}

void LuaTransaction::finishTransaction() noexcept {
    finished_ = true;
    transaction_.reset();
}

// === Internal Execution Helpers ===
std::shared_ptr<LuaResult> LuaTransaction::executeQueryInternal(const std::string& sql, const luabridge::LuaRef* params) {
    ensureValid();
    if (params && !params->isNil() && !params->isTable())
        throw std::runtime_error("Database query parameters must be a Lua table");

    return withErrorHandling([&] {
        if (!params || params->isNil()) {
            return std::make_shared<LuaResult>(transaction_->execSqlSync(sql));
        }

        auto binder = (*transaction_) << sql;
        LuaDbUtils::bindLuaParameters(binder, *params);
        binder << drogon::orm::Mode::Blocking;
        drogon::orm::Result result(nullptr);
        binder >> [&result](const drogon::orm::Result& r) { result = r; };
        binder.exec();
        
        return std::make_shared<LuaResult>(std::move(result));
    });
}

void LuaTransaction::executeQueryAsyncInternal(const std::string& sql, const luabridge::LuaRef* params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    ensureValid();
    if (!callback) throw std::runtime_error("Lua transaction async query callback is empty");
    if (!errorCallback) throw std::runtime_error("Lua transaction async error callback is empty");
    if (params && !params->isNil() && !params->isTable()) 
        throw std::runtime_error("Database query parameters must be a Lua table");

    if (!params || params->isNil()) {
        transaction_->execSqlAsync(sql,
            [callback](const drogon::orm::Result& result) { 
                callback(std::make_shared<LuaResult>(result)); 
            },

            [errorCallback](const drogon::orm::DrogonDbException& e) { 
                errorCallback(e.base().what()); 
            }
        );

    } else {
        auto binder = (*transaction_) << sql;
        LuaDbUtils::bindLuaParameters(binder, *params);
        binder >> [callback](const drogon::orm::Result& result) { 
            callback(std::make_shared<LuaResult>(result)); 
        };
        binder >> [errorCallback](const drogon::orm::DrogonDbException& e) { 
            errorCallback(e.base().what()); 
        };
        binder.exec();
    }
}