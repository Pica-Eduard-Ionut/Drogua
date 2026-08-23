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