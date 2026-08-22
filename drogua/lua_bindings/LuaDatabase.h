#pragma once

#include <drogon/orm/DbClient.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <memory>
#include <string>

class LuaResult;

class LuaDatabase {
    public:
        explicit LuaDatabase(const std::string& name = "default");

        static std::shared_ptr<LuaDatabase> get(const std::string& name);

        const std::string& name() const;
        bool valid() const;

        std::shared_ptr<LuaResult> execute(const std::string& sql);
        std::shared_ptr<LuaResult> query(const std::string& sql);
        // Parameterized overload allowing for `db.query(sql_statement, {param1, param2, ...})` from Lua
        std::shared_ptr<LuaResult> query(const std::string& sql, const luabridge::LuaRef& params);
        // Lua-facing wrapper
        std::shared_ptr<LuaResult> queryLua(const std::string& sql, const luabridge::LuaRef& params);

        std::size_t executeAffected(const std::string& sql);
        unsigned long long lastInsertId(const std::string& sql);

    private:
        std::string name_;
        drogon::orm::DbClientPtr client_;
};