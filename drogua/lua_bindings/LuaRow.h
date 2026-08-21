#pragma once

#include <drogon/orm/Row.h>
#include <string>

struct lua_State;

class LuaRow {
    public:
        explicit LuaRow(const drogon::orm::Row &row);

        // Number of columns in this row.
        std::size_t size() const;

        // Get column name by zero-based index.
        std::string columnName(std::size_t index) const;

        // Check whether a column is NULL using the name as a `STRING`
        bool isNull(const std::string &column) const;
        // Check whether a column is NULL using the index as an `INT`
        bool isNull(std::size_t index) const;

        // Push a column value onto the Lua stack.
        // Supported conversions:
        //   - NULL      -> nil
        //   - bool      -> boolean
        //   - integers  -> integer
        //   - floating  -> number
        //   - everything else -> string
        //
        // This is mostly useful internally for LuaBridge.
        void pushValue(lua_State *L, const std::string &column) const;
        void pushValue(lua_State *L, std::size_t index) const;

        // Return a value to Lua through LuaBridge.
        //
        // Rather than exposing this directly as a C++ return type, the
        // LuaBridge registration below uses a custom Lua function.
        std::string getString(const std::string &column) const;
        std::string getString(std::size_t index) const;

        // Useful for debugging/displaying rows.
        std::string toString() const;

    private:
        drogon::orm::Row row_;
};