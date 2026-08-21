#include "LuaDatabase.h"
#include "LuaResult.h"

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

std::size_t LuaDatabase::executeAffected(const std::string& sql) {
    return execute(sql)->affectedRows();
}

unsigned long long LuaDatabase::lastInsertId(const std::string& sql) {
    return execute(sql)->insertId();
}

std::shared_ptr<LuaDatabase> LuaDatabase::get(const std::string& name) {
    return std::make_shared<LuaDatabase>(name);
}