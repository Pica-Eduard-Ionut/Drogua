#include "LuaDatabase.h"
#include "LuaDbUtils.h"

#include <drogon/drogon.h>

#include <stdexcept>

LuaDatabase::LuaDatabase(const std::string& name) : name_(name) {
    client_ = drogon::app().getDbClient(name_);

    if (!client_)
        throw std::runtime_error("Drogon database client '" + name_ + "' does not exist");
}

const std::string& LuaDatabase::name() const {
    return name_;
}

bool LuaDatabase::valid() const {
    return static_cast<bool>(client_);
}

std::shared_ptr<LuaResult> LuaDatabase::execute(const std::string& sql) {
    LuaDatabase::checkClient();
    return runDbOperation("Database error", [&] {
        return std::make_shared<LuaResult>(client_->execSqlSync(sql));
    });
}

std::shared_ptr<LuaResult> LuaDatabase::query(const std::string& sql) {
    return execute(sql);
}

// overload for query
std::shared_ptr<LuaResult> LuaDatabase::query(const std::string& sql, const luabridge::LuaRef& params) {
    LuaDatabase::checkClient();
    if (!params.isTable()) 
        throw std::runtime_error("Database query parameters must be a Lua table");

    return runDbOperation("Database error", [&] {
        auto binder = (*client_) << sql;
        LuaDbUtils::bindLuaParameters(binder, params);
        binder << drogon::orm::Mode::Blocking;
        drogon::orm::Result result(nullptr);
        binder >> [&result](const drogon::orm::Result& r) { result = r; };
        binder.exec();

        return std::make_shared<LuaResult>(std::move(result));
    });
}


// Lua facing wrapper
std::shared_ptr<LuaResult> LuaDatabase::queryLua(const std::string& sql, const luabridge::LuaRef& params) {
    return params.isNil() ? query(sql) : query(sql, params);
}

std::size_t LuaDatabase::executeAffected(const std::string& sql) {
    return execute(sql)->affectedRows();
}

unsigned long long LuaDatabase::lastInsertId(const std::string& sql) {
    return execute(sql)->insertId();
}

std::shared_ptr<LuaDatabase> LuaDatabase::get(const std::string& name) {
    return std::make_shared<LuaDatabase>(name);
}

std::shared_ptr<LuaTransaction> LuaDatabase::begin() {
    LuaDatabase::checkClient();

    return runDbOperation("Failed to begin transaction", [&] {
        auto transaction = client_->newTransaction([](bool success) {
            if (success) LOG_TRACE << "Lua transaction committed";
            
            else LOG_ERROR << "Lua transaction commit failed";
        });

        return std::make_shared<LuaTransaction>(std::move(transaction));
    });
}

void LuaDatabase::queryAsync(const std::string& sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    LuaDatabase::checkClient();
    validateAsyncCallbacks(callback, errorCallback);
    client_->execSqlAsync(sql,
        [callback](const drogon::orm::Result& result) 
            { callback(std::make_shared<LuaResult>(result)); },

        [errorCallback](const drogon::orm::DrogonDbException& e) 
            { errorCallback(e.base().what()); }
    );
}

void LuaDatabase::queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    LuaDatabase::checkClient();
    validateAsyncCallbacks(callback, errorCallback);
    if (!params.isTable()) throw std::runtime_error("Database query parameters must be a Lua table");

    auto binder = (*client_) << sql;
    LuaDbUtils::bindLuaParameters(binder, params);
    binder >> [callback](const drogon::orm::Result& result) { callback(std::make_shared<LuaResult>(result)); };
    binder >> [errorCallback](const drogon::orm::DrogonDbException& e) { errorCallback(e.base().what()); };
    binder.exec();
}

int LuaDatabase::queryAsyncLua(lua_State* L) {
    auto [db, routeContext] = LuaDbUtils::extractAsyncLuaArgs<LuaDatabase>(L, "Database queryAsync");
    const char* sql = luaL_checkstring(L, 2);
    const int argc = lua_gettop(L);
    if (argc < 2 || argc > 3) 
        return luaL_error(L, "Database queryAsync expects sql and optional parameters");

    auto context = std::make_shared<LuaAsyncDatabaseContext>();
    context->coroutine = routeContext->coroutine;
    context->callback = routeContext->callback;
    context->resume = routeContext->resume;
    context->request = routeContext->request;
    LuaAsyncContextRegistry::set<LuaAsyncDatabaseContext>(L, context);

    auto successCb = [context](std::shared_ptr<LuaResult> result) {
        if (context->request && !context->request->connected()) return;
        context->asyncResult = std::move(result);
        context->asyncError.clear();
        if (context->resume) context->resume();
        else LOG_ERROR << "Async database query completed, but no resume handler installed";
    };

    auto errorCb = [context](const std::string& error) {
        if (context->request && !context->request->connected()) return;
        context->asyncResult.reset();
        context->asyncError = error;
        if (context->resume) context->resume();
        else LOG_ERROR << "Async database query failed, but no resume handler installed";
    };

    try {
        if (argc == 3) {
            if (!lua_istable(L, 3)) { 
                LuaDbUtils::clearAsyncContext<LuaAsyncDatabaseContext>(L); 
                return luaL_error(L, "Parameters must be a table"); 
            }

            auto paramsResult = luabridge::Stack<luabridge::LuaRef>::get(L, 3);
            if (!paramsResult) { 
                LuaDbUtils::clearAsyncContext<LuaAsyncDatabaseContext>(L); 
                return luaL_error(L, "Invalid parameters: %s", paramsResult.message().c_str()); 
            }
            db->queryAsync(sql, paramsResult.value(), successCb, errorCb);

        } else {
            db->queryAsync(sql, successCb, errorCb);
        }

    } catch (const std::exception& e) {
        LuaDbUtils::clearAsyncContext<LuaAsyncDatabaseContext>(L);
        return luaL_error(L, "Async database query failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaDatabase::queryAsyncContinuation);
}

int LuaDatabase::queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    return LuaDbUtils::handleQueryContinuation<LuaAsyncDatabaseContext>(L, status, ctx, "database query");
}

// async transaction
void LuaDatabase::beginAsync(std::function<void(std::shared_ptr<LuaTransaction>)> callback, std::function<void(const std::string&)> errorCallback) {
    LuaDatabase::checkClient();
    validateAsyncCallbacks(callback, errorCallback);

    try {
        client_->newTransactionAsync([callback, errorCallback](const std::shared_ptr<drogon::orm::Transaction>& transaction) {
            if (!transaction) { 
                errorCallback("Failed to create database transaction"); 
                return; 
            }
            callback(std::make_shared<LuaTransaction>(transaction));
        });
    }

    catch (const std::exception& e) {
        errorCallback(e.what());
    }
}

int LuaDatabase::beginAsyncLua(lua_State* L) {
    auto [db, routeContext] = LuaDbUtils::extractAsyncLuaArgs<LuaDatabase>(L, "Database beginAsync");
    
    auto transactionContext = std::make_shared<LuaAsyncTransactionContext>();
    transactionContext->coroutine = routeContext->coroutine;
    transactionContext->callback = routeContext->callback;
    transactionContext->resume = routeContext->resume;
    transactionContext->request = routeContext->request;
    LuaAsyncContextRegistry::set<LuaAsyncTransactionContext>(L, transactionContext);

    auto successCb = [transactionContext](std::shared_ptr<LuaTransaction> transaction) {
        if (transactionContext->request && !transactionContext->request->connected()) return;
        transactionContext->asyncError.clear();
        transactionContext->transaction = std::move(transaction);
        if (transactionContext->resume) transactionContext->resume();
    };

    auto errorCb = [transactionContext](const std::string& error) {
        if (transactionContext->request && !transactionContext->request->connected()) return;
        transactionContext->transaction.reset();
        transactionContext->asyncError = error;
        if (transactionContext->resume) transactionContext->resume();
    };

    try {
        db->beginAsync(successCb, errorCb);

    } catch (const std::exception& e) {
        LuaDbUtils::clearAsyncContext<LuaAsyncTransactionContext>(L);
        return luaL_error(L, "Async transaction begin failed: %s", e.what());
    }
    
    return lua_yieldk(L, 0, 0, &LuaDatabase::beginAsyncContinuation);
}

int LuaDatabase::beginAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    return LuaDbUtils::handleBeginTransactionContinuation<LuaAsyncTransactionContext>(L, status, ctx, "transaction begin");
}

void LuaDatabase::checkClient() const {
    if (!client_) throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
}
