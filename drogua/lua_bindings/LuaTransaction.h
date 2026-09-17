#pragma once

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <drogon/orm/DbClient.h>

#include <memory>
#include <string>
#include <functional>

class LuaResult;

void bindLuaParameters(drogon::orm::internal::SqlBinder& binder, const luabridge::LuaRef& params);

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

        // async
        void queryAsync(const std::string& sql, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback);
        void queryAsync(const std::string& sql, const luabridge::LuaRef& params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback);
        static int queryAsyncLua(lua_State* L);
        static int queryAsyncContinuation(lua_State* L, int status, lua_KContext ctx);

    private:
        std::shared_ptr<drogon::orm::Transaction> transaction_;
        bool finished_{false};

        // validation and cleanup helpers
        void ensureValid() const;
        void ensureParamsTable(const luabridge::LuaRef& params) const;
        void finishTransaction() noexcept;
        // execution helpers
        std::shared_ptr<LuaResult> executeQueryInternal(const std::string& sql, const luabridge::LuaRef* params);
        void executeQueryAsyncInternal(const std::string& sql, const luabridge::LuaRef* params, std::function<void(std::shared_ptr<LuaResult>)> callback, std::function<void(const std::string&)> errorCallback);
        // error handling wrapper
        template<typename Func>
        auto withErrorHandling(Func&& func) -> decltype(func()) {
            try {
                return func();
            }

            catch (const drogon::orm::DrogonDbException& e) {
                throw std::runtime_error("Transaction database error: " + std::string(e.base().what()));
            }
            
            catch (const std::exception& e) {
                throw std::runtime_error("Transaction database error: " + std::string(e.what()));
            }
        }
};
