#pragma once

#include <drogon/orm/DbClient.h>

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

        std::size_t executeAffected(const std::string& sql);
        unsigned long long lastInsertId(const std::string& sql);

    private:
        std::string name_;
        drogon::orm::DbClientPtr client_;
};