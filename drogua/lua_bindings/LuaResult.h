#pragma once

#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>

#include <memory>
#include <string>

class LuaRow;

struct lua_State;

class LuaResult {
    public:
        explicit LuaResult(const drogon::orm::Result &result);

        // Number of returned rows.
        std::size_t size() const;

        // Alias that's nicer from Lua.
        std::size_t count() const;

        // Number of columns.
        std::size_t columns() const;

        // Get a column name by zero-based index.
        std::string columnName(std::size_t index) const;

        // Get a row by zero-based index
        std::shared_ptr<LuaRow> row(std::size_t index) const;

        // Number of rows affected by INSERT/UPDATE/DELETE.
        std::size_t affectedRows() const;

        // Auto-increment ID where Drogon supports it.
        unsigned long long insertId() const;

        // Convert the result to a Lua array of row tables.
        void pushTable(lua_State* L) const;
        // Lua-facing wrapper.
        static int luaToTable(lua_State* L);

        // Print/debug representation.
        std::string toString() const;

    private:
        drogon::orm::Result result_;
};