#pragma once

#include <drogon/orm/DbClient.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include "LuaResult.h"
#include "LuaTransaction.h"
#include "LuaAsyncContext.h"
#include "LuaAsyncContextRegistry.h"

#include <memory>
#include <string>
#include <functional>

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
        
        std::shared_ptr<LuaTransaction> begin();

        // async transaction
        void beginAsync(std::function<void(std::shared_ptr<LuaTransaction>)> callback, std::function<void(const std::string&)> errorCallback);
        static int beginAsyncLua(lua_State* L);
        static int beginAsyncContinuation(lua_State* L, int status, lua_KContext ctx);

        // async
        void queryAsync(const std::string& sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback);
        // Parametrized overload
        void queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, 
            std::function<void(const std::string&)> errorCallback);
        // Lua-facing async query
        static int queryAsyncLua(lua_State* L);
        static int queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx);

    private:
        std::string name_;
        drogon::orm::DbClientPtr client_;
};