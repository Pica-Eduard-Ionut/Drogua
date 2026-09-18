#include "LuaDatabase.h"

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
    if (!params.isTable()) throw std::runtime_error("Database query parameters must be a Lua table");

    return runDbOperation("Database error", [&] {
        auto binder = (*client_) << sql;
        bindLuaParameters(binder, params);
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
    bindLuaParameters(binder, params);
    binder >> [callback](const drogon::orm::Result& result) { callback(std::make_shared<LuaResult>(result)); };
    binder >> [errorCallback](const drogon::orm::DrogonDbException& e) { errorCallback(e.base().what()); };
    binder.exec();
}

int LuaDatabase::queryAsyncLua(lua_State* L) {
    auto [db, routeContext] = getAsyncContexts(L, "queryAsync");
    const char* sql = luaL_checkstring(L, 2);
    const int argc = lua_gettop(L);
    if (argc < 2 || argc > 3) return luaL_error(L, "Database queryAsync expects sql and optional parameters");

    auto context = std::make_shared<LuaAsyncDatabaseContext>();
    context->coroutine = routeContext->coroutine;
    context->callback = routeContext->callback;
    context->resume = routeContext->resume;
    context->request = routeContext->request;
    LuaAsyncContextRegistry::set<LuaAsyncDatabaseContext>(L, context);
    // success callback
    auto successCb = [context](std::shared_ptr<LuaResult> result) {
        if (context->request && !context->request->connected()) return;

        context->asyncResult = std::move(result);
        context->asyncError.clear();
        if (context->resume) context->resume();

        else LOG_ERROR << "Lua async database query completed, but no resume handler is installed";
    };
    // error callback
    auto errorCb = [context](const std::string& error) {
        if (context->request && !context->request->connected()) return;

        context->asyncResult.reset();
        context->asyncError = error;
        if (context->resume) context->resume();

        else LOG_ERROR << "Lua async database query failed, but no resume handler is installed";
    };

    try {
        if (argc == 3) {
            if (!lua_istable(L, 3)) { 
                clearAsyncDatabaseContext(L, context); 
                return luaL_error(L, "Database queryAsync parameters must be a table"); 
            }

            auto paramsResult = luabridge::Stack<luabridge::LuaRef>::get(L, 3);
            if (!paramsResult) { 
                clearAsyncDatabaseContext(L, context);
                return luaL_error(L, "Invalid query parameters: %s", paramsResult.message().c_str()); 
            }
            db->queryAsync(sql, paramsResult.value(), successCb, errorCb);

        } else {
            db->queryAsync(sql, successCb, errorCb);
        }
    }
    catch (const std::exception& e) {
        clearAsyncDatabaseContext(L, context);
        return luaL_error(L, "Async database query failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaDatabase::queryAsyncContinuation);
}

int LuaDatabase::queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;
    if (!L) return 0;

    auto context = LuaAsyncContextRegistry::get<LuaAsyncDatabaseContext>(L);
    if (!context) 
        return luaL_error(L, "Database queryAsync continuation has no database async context");

    if (status != LUA_YIELD) { 
        clearAsyncDatabaseContext(L, context); 
        return luaL_error(L, "Database queryAsync continuation resumed with unexpected status"); 
    }

    if (!context->asyncError.empty()) {
        const std::string error = context->asyncError;
        context->asyncError.clear();
        clearAsyncDatabaseContext(L, context);
        return luaL_error(L, "Async database query failed: %s", error.c_str());
    }

    if (!context->asyncResult) { 
        clearAsyncDatabaseContext(L, context); 
        return luaL_error(L, "Async database query completed without a result"); 
    }

    auto result = context->asyncResult;
    context->asyncResult.reset();
    clearAsyncDatabaseContext(L, context);

    auto pushResult = luabridge::Stack<std::shared_ptr<LuaResult>>::push(L, result);
    if (!pushResult) 
        return luaL_error(L, "Failed to push async database result: %s", pushResult.message().c_str());

    return 1;
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
    auto [db, routeContext] = getAsyncContexts(L, "beginAsync");
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
    }

    catch (const std::exception& e) {
        LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L);
        return luaL_error(L, "Async transaction begin failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaDatabase::beginAsyncContinuation);
}

int LuaDatabase::beginAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;
    if (!L) return 0;

    auto context = LuaAsyncContextRegistry::get<LuaAsyncTransactionContext>(L);
    if (!context) return luaL_error(L, "Database beginAsync continuation has no transaction async context");

    if (status != LUA_YIELD) { 
        LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L); 
        return luaL_error(L, "Database beginAsync continuation resumed with unexpected status"); 
    }

    if (!context->asyncError.empty()) {
        const std::string error = context->asyncError;
        context->asyncError.clear();
        LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L);

        return luaL_error(L, "Async transaction begin failed: %s", error.c_str());
    }

    if (!context->transaction) { 
        LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L); 
        return luaL_error(L, "Async transaction begin completed without a transaction"); 
    }

    auto transaction = context->transaction;
    context->transaction.reset();
    LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L);
    auto pushResult = luabridge::Stack<std::shared_ptr<LuaTransaction>>::push(L, transaction);
    if (!pushResult) 
        return luaL_error(L, "Failed to push async transaction: %s", pushResult.message().c_str());

    return 1;
}

void LuaDatabase::clearAsyncDatabaseContext(lua_State* L, const std::shared_ptr<LuaAsyncDatabaseContext>& context) {
    if (!L || !context) {
        return;
    }

    LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
}

LuaDatabase::AsyncSetup LuaDatabase::getAsyncContexts(lua_State* L, const char* funcName) {
    if (!L || !lua_isuserdata(L, 1)) {
        luaL_error(L, "Database %s expected a DatabaseClient", funcName);
    }
    
    auto database = luabridge::get<LuaDatabase*>(L, 1);
    if (!database) {
        luaL_error(L, "Invalid DatabaseClient: %s", database.message().c_str());
    }
    if (!database.value()) {
        luaL_error(L, "DatabaseClient is null");
    }

    auto routeContext = LuaAsyncContextRegistry::get<LuaAsyncRouteContext>(L);
    if (!routeContext) {
        luaL_error(L, "Database %s must be called from an async route", funcName);
    }
    if (!routeContext->coroutine) {
        luaL_error(L, "Database %s has no active coroutine", funcName);
    }
    
    return {database.value(), routeContext};
}

void LuaDatabase::checkClient() const {
    if (!client_) throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
}
