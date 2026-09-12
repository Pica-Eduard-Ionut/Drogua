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
    if (!client_)
        throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");

    try {
        return std::make_shared<LuaResult>(client_->execSqlSync(sql));
    }

    catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error("Database error [" + name_ + "]: " + std::string(e.base().what()));
    }

    catch (const std::exception& e) {
        throw std::runtime_error("Database error [" + name_ + "]: " + std::string(e.what()));
    }
}

std::shared_ptr<LuaResult> LuaDatabase::query(const std::string& sql) {
    return execute(sql);
}

// overload for query
std::shared_ptr<LuaResult> LuaDatabase::query(const std::string& sql, const luabridge::LuaRef& params) {
    if (!client_) throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
    if (!params.isTable()) throw std::runtime_error("Database query parameters must be a Lua table");

    try {
        auto binder = (*client_) << sql;
        bindLuaParameters(binder, params);
        binder << drogon::orm::Mode::Blocking;

        drogon::orm::Result result(nullptr);
        binder >> [&result](const drogon::orm::Result& r) { result = r; };
        binder.exec();

        return std::make_shared<LuaResult>(std::move(result));
    
    } catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error("Database error [" + name_ + "]: " + std::string(e.base().what()));
    
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error [" + name_ + "]: " + std::string(e.what()));
    }
}

// Lua facing wrapper
std::shared_ptr<LuaResult> LuaDatabase::queryLua(const std::string& sql, const luabridge::LuaRef& params) {
    if (params.isNil()) {
        return query(sql);
    }

    return query(sql, params);
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
    if (!client_) {
        throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
    }

    try {
        auto transaction = client_->newTransaction([](bool success) {
                if (success) {
                    LOG_TRACE << "Lua transaction committed";
                }

                else {
                    LOG_ERROR << "Lua transaction commit failed";
                }
            }
        );

        return std::make_shared<LuaTransaction>(
            std::move(transaction)
        );
    }

    catch (const drogon::orm::DrogonDbException& e) {
        throw std::runtime_error("Failed to begin transaction [" + name_ + "]: " + std::string(e.base().what()));
    }

    catch (const std::exception& e) {
        throw std::runtime_error("Failed to begin transaction [" + name_ + "]: " + std::string(e.what()));
    }
}

void LuaDatabase::queryAsync(const std::string &sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string &)> errorCallback) {
    if (!client_) {
        throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
    }

    if (!callback) {
        throw std::runtime_error("LuaDatabase async query callback is empty");
    }

    if (!errorCallback) {
        throw std::runtime_error("LuaDatabase async error callback is empty");
    }

    client_->execSqlAsync(
        sql,
        [callback](const drogon::orm::Result &result) {
            callback(std::make_shared<LuaResult>(result));
        },
        [errorCallback](const drogon::orm::DrogonDbException &e) {
            errorCallback(e.base().what());
        }
    );
}

void LuaDatabase::queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback) {
    if (!client_) {
        throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
    }
    if (!params.isTable()) {
        throw std::runtime_error("Database query parameters must be a Lua table");
    }
    if (!callback) {
        throw std::runtime_error("LuaDatabase async query callback is empty");
    }
    if (!errorCallback) {
        throw std::runtime_error("LuaDatabase async error callback is empty");
    }

    auto binder = (*client_) << sql;
    bindLuaParameters(binder, params);

    binder >> [callback](const drogon::orm::Result& result) {
        callback(std::make_shared<LuaResult>(result));
    };

    binder >> [errorCallback](const drogon::orm::DrogonDbException& e) {
        errorCallback(e.base().what());
    };

    binder.exec();
}

int LuaDatabase::queryAsyncLua(lua_State* L) {
    if (!L) {
        return luaL_error(L, "Database queryAsync received null Lua state");
    }

    if (!lua_isuserdata(L, 1)) {
        return luaL_error(L, "Database queryAsync expected a DatabaseClient");
    }

    auto database = luabridge::get<LuaDatabase*>(L, 1);
    if (!database) {
        return luaL_error(L, "Invalid DatabaseClient: %s", database.message().c_str());
    }

    LuaDatabase* db = database.value();
    if (!db) {
        return luaL_error(L, "DatabaseClient is null");
    }

    const char* sql = luaL_checkstring(L, 2);
    const int argc = lua_gettop(L);
    if (argc < 2 || argc > 3) {
        return luaL_error(L, "Database queryAsync expects sql and optional parameters");
    }

    /*
     * queryAsync() must be called from an async Lua route.
     * The route context provides the coroutine/resume machinery.
     */
    auto routeContext = LuaAsyncContextRegistry::get<LuaAsyncRouteContext>(L);
    if (!routeContext) {
        return luaL_error(L, "Database queryAsync must be called from an async route");
    }

    if (!routeContext->coroutine) {
        return luaL_error(L, "Database queryAsync has no active coroutine");
    }

    // Create the specialized database context.
    auto context = std::make_shared<LuaAsyncDatabaseContext>();
    context->coroutine = routeContext->coroutine;
    context->callback = routeContext->callback;
    context->resume = routeContext->resume;
    // Register the database context separately from the route context.
    LuaAsyncContextRegistry::set<LuaAsyncDatabaseContext>(L, context);

    try {
        if (argc == 3) {
            if (!lua_istable(L, 3)) {
                LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
                return luaL_error(L, "Database queryAsync parameters must be a table");
            }

            auto paramsResult = luabridge::Stack<luabridge::LuaRef>::get(L, 3);

            if (!paramsResult) {
                LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
                return luaL_error(L, "Invalid query parameters: %s", paramsResult.message().c_str());
            }

            auto params = paramsResult.value();
            db->queryAsync(sql, params, [context](std::shared_ptr<LuaResult> result) {
                context->asyncResult = std::move(result);
                context->asyncError.clear();

                if (context->resume) {
                    context->resume();
                } else {
                    LOG_ERROR << "Lua async database query completed, but no resume handler is installed";
                }

            }, [context](const std::string& error) {
                context->asyncResult.reset();
                context->asyncError = error;

                if (context->resume) {
                    context->resume();
                } else {
                    LOG_ERROR << "Lua async database query failed, but no resume handler is installed";
                }
            });

        } else {
            db->queryAsync(sql, [context](std::shared_ptr<LuaResult> result) {
                context->asyncResult = std::move(result);
                context->asyncError.clear();

                if (context->resume) {
                    context->resume();
                } else {
                    LOG_ERROR << "Lua async database query completed, but no resume handler is installed";
                }

            }, [context](const std::string& error) {
                context->asyncResult.reset();
                context->asyncError = error;

                if (context->resume) {
                    context->resume();
                } else {
                    LOG_ERROR << "Lua async database query failed, but no resume handler is installed";
                }
            });
        }
    }

    catch (const std::exception& e) {
        LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
        return luaL_error(L, "Async database query failed: %s", e.what());
    }

    /*
     * Suspend the Lua coroutine.
     * The database callback will eventually call context->resume().
     */
    return lua_yieldk(L, 0, 0, &LuaDatabase::queryAsyncContinuation);
}

int LuaDatabase::queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;

    if (!L) {
        return 0;
    }

    auto context = LuaAsyncContextRegistry::get<LuaAsyncDatabaseContext>(L);
    if (!context) {
        return luaL_error(L, "Database queryAsync continuation has no database async context");
    }

    if (status != LUA_YIELD) {
        LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
        return luaL_error(L, "Database queryAsync continuation resumed with unexpected status");
    }

    // Database operation failed.
    if (!context->asyncError.empty()) {
        const std::string error = context->asyncError;
        context->asyncError.clear();
        LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
        return luaL_error(L, "Async database query failed: %s", error.c_str());
    }

    // Database operation succeeded, but no result was stored.
    if (!context->asyncResult) {
        LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
        return luaL_error(L, "Async database query completed without a result");
    }

    // Take ownership of the result locally.
    auto result = context->asyncResult;
    context->asyncResult.reset();

    // The database operation is finished, so remove the specialized database context from the registry.
    LuaAsyncContextRegistry::clear<LuaAsyncDatabaseContext>(L);
    // Push LuaResult as the return value of db:queryAsync().
    auto pushResult = luabridge::Stack<std::shared_ptr<LuaResult>>::push(L, result);
    if (!pushResult) {
        return luaL_error(L, "Failed to push async database result: %s", pushResult.message().c_str());
    }

    return 1;
}

// async transaction
void LuaDatabase::beginAsync(std::function<void(std::shared_ptr<LuaTransaction>)> callback, std::function<void(const std::string&)> errorCallback) {
    if (!client_) {
        throw std::runtime_error("LuaDatabase '" + name_ + "' has no valid Drogon DbClient");
    }

    if (!callback) {
        throw std::runtime_error("LuaDatabase async transaction callback is empty");
    }

    if (!errorCallback) {
        throw std::runtime_error("LuaDatabase async transaction error callback is empty");
    }

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
    if (!L) {
        return luaL_error(L, "Database beginAsync received null Lua state");
    }

    if (!lua_isuserdata(L, 1)) {
        return luaL_error(L, "Database beginAsync expected a DatabaseClient");
    }

    auto database = luabridge::get<LuaDatabase*>(L, 1);
    if (!database) {
        return luaL_error(L, "Invalid DatabaseClient: %s", database.message().c_str());
    }

    LuaDatabase* db = database.value();
    if (!db) {
        return luaL_error(L, "DatabaseClient is null");
    }

    // beginAsync is started from the route context.
    auto routeContext = LuaAsyncContextRegistry::get<LuaAsyncRouteContext>(L);
    if (!routeContext) {
        return luaL_error(L, "Database beginAsync must be called from an async route");
    }

    if (!routeContext->coroutine) {
        return luaL_error(L, "Database beginAsync has no active coroutine");
    }

    // Create the transaction context.
    auto transactionContext = std::make_shared<LuaAsyncTransactionContext>();
    transactionContext->coroutine = routeContext->coroutine;
    transactionContext->callback = routeContext->callback;
    transactionContext->resume = routeContext->resume;
    // Register the transaction context.
    LuaAsyncContextRegistry::set<LuaAsyncTransactionContext>(L, transactionContext);

    try {
        db->beginAsync([transactionContext](std::shared_ptr<LuaTransaction> transaction) {
            transactionContext->asyncError.clear();
            transactionContext->transaction = std::move(transaction);

            if (transactionContext->resume) {
                transactionContext->resume();
            }
        }, [transactionContext](const std::string& error) {
            transactionContext->transaction.reset();
            transactionContext->asyncError = error;

            if (transactionContext->resume) {
                transactionContext->resume();
            }
        });
    }

    catch (const std::exception& e) {
        LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L);
        return luaL_error(L, "Async transaction begin failed: %s", e.what());
    }

    return lua_yieldk(L, 0, 0, &LuaDatabase::beginAsyncContinuation);
}

int LuaDatabase::beginAsyncContinuation(lua_State* L, int status, lua_KContext ctx) {
    (void)ctx;

    if (!L) {
        return 0;
    }

    auto context = LuaAsyncContextRegistry::get<LuaAsyncTransactionContext>(L);
    if (!context) {
        return luaL_error(L, "Database beginAsync continuation has no transaction async context");
    }

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

    /*
     * The transaction object is now owned by Lua.
     * The temporary async transaction context is no longer needed for beginAsync().
     */
    LuaAsyncContextRegistry::clear<LuaAsyncTransactionContext>(L);
    auto pushResult = luabridge::Stack<std::shared_ptr<LuaTransaction>>::push(L, transaction);
    if (!pushResult) {
        return luaL_error(L, "Failed to push async transaction: %s", pushResult.message().c_str());
    }

    return 1;
}
