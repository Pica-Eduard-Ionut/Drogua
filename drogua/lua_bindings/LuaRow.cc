#include "LuaRow.h"

#include <drogon/orm/Field.h>
#include <lua.hpp>

#include <sstream>
#include <stdexcept>

LuaRow::LuaRow(const drogon::orm::Row& row) : row_(row) {}

std::size_t LuaRow::size() const {
    return row_.size();
}

std::string LuaRow::columnName(std::size_t index) const {
    if (index >= row_.size())
        throw std::out_of_range("LuaRow column index out of range");

    return row_[index].name();
}

bool LuaRow::isNull(const std::string& column) const {
    return row_[column].isNull();
}

bool LuaRow::isNull(std::size_t index) const {
    if (index >= row_.size())
        throw std::out_of_range("LuaRow column index out of range");

    return row_[index].isNull();
}

std::string LuaRow::getString(const std::string& column) const {
    const auto field = row_[column];
    return field.isNull() ? std::string{} : field.as<std::string>();
}

std::string LuaRow::getString(std::size_t index) const {
    if (index >= row_.size())
        throw std::out_of_range("LuaRow column index out of range");

    const auto field = row_[index];
    return field.isNull() ? std::string{} : field.as<std::string>();
}

void LuaRow::pushValue(lua_State* L, const std::string& column) const {
    const auto field = row_[column];
    if (field.isNull()) {
        lua_pushnil(L);
        return;
    }

    const std::string value = field.as<std::string>();
    lua_pushlstring(L, value.data(), value.size());
}

std::string LuaRow::toString() const {
    std::ostringstream ss;
    ss << "{";

    for (std::size_t i = 0; i < row_.size(); ++i) {
        if (i > 0)
            ss << ", ";

        ss << row_[i].name() << "=";

        if (row_[i].isNull())
            ss << "NULL";
            
        else
            ss << row_[i].as<std::string>();
    }

    ss << "}";
    return ss.str();
}