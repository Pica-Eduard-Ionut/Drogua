#pragma once

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <drogon/orm/DbClient.h>

#include <memory>
#include <string>

class LuaResult;

class LuaTransaction {
    public:
        explicit LuaTransaction(std::shared_ptr<drogon::orm::Transaction> transaction);
        ~LuaTransaction();

        LuaTransaction(const LuaTransaction&) = delete;
        LuaTransaction& operator=(const LuaTransaction&) = delete;

        bool valid() const;

        std::shared_ptr<LuaResult> query(const std::string& sql);
        std::shared_ptr<LuaResult> query(const std::string& sql, const luabridge::LuaRef& params);
        std::shared_ptr<LuaResult> queryLua(const std::string& sql, const luabridge::LuaRef& params);

        std::size_t executeAffected(const std::string& sql);
        unsigned long long lastInsertId(const std::string& sql);

        void commit();
        void rollback();

    private:
        std::shared_ptr<drogon::orm::Transaction> transaction_;
        bool finished_{false};
};
