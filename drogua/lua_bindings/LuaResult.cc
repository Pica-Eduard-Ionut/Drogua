#include "LuaResult.h"
#include "LuaRow.h"

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <sstream>
#include <stdexcept>

LuaResult::LuaResult(const drogon::orm::Result& result) : result_(result) {}

std::size_t LuaResult::size() const {
    return result_.size();
}

std::size_t LuaResult::count() const {
    return result_.size();
}

std::size_t LuaResult::columns() const {
    return result_.columns();
}

std::string LuaResult::columnName(std::size_t index) const {
    if (index >= result_.columns())
        throw std::out_of_range("LuaResult column index out of range");

    return result_.columnName(index);
}

std::shared_ptr<LuaRow> LuaResult::row(std::size_t index) const {
    if (index >= result_.size())
        throw std::out_of_range("LuaResult row index out of range");

    return std::make_shared<LuaRow>(result_[index]);
}

std::size_t LuaResult::affectedRows() const {
    return result_.affectedRows();
}

unsigned long long LuaResult::insertId() const {
    return result_.insertId();
}

void LuaResult::pushTable(lua_State* L) const {
    lua_createtable(L, static_cast<int>(result_.size()), 0);

    for (std::size_t rowIndex = 0; rowIndex < result_.size(); ++rowIndex) {
        const auto row = result_[rowIndex];
        lua_createtable(L, 0, static_cast<int>(result_.columns()));

        for (std::size_t columnIndex = 0; columnIndex < result_.columns(); ++columnIndex) {
            const auto field = row[columnIndex];
            const char* name = field.name();
            if (field.isNull()) {
                lua_pushnil(L);

            } else {
                const std::string value = field.as<std::string>();
                lua_pushlstring(L, value.data(), value.size());
            }

            lua_setfield(L, -2, name);
        }

        lua_rawseti(L, -2, static_cast<lua_Integer>(rowIndex + 1));
    }
}

int LuaResult::luaToTable(lua_State* L) {
    auto result = luabridge::get<LuaResult*>(L, 1);
    if (!result) {
        lua_pushstring(L, "Invalid DatabaseResult");
        return lua_error(L);
    }

    result.value()->pushTable(L);
    return 1;
}

std::string LuaResult::toString() const {
    std::ostringstream output;
    output << "LuaResult(" << result_.size() << " rows, " << result_.columns() << " columns)";
    return output.str();
}