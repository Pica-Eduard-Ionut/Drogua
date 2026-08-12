#include "LuaRequest.h"

LuaRequest::LuaRequest(const drogon::HttpRequestPtr &request)
    : request_(request)
{ }

luabridge::LuaRef LuaRequest::params(lua_State *L) const {
    luabridge::LuaRef result = luabridge::newTable(L);

    for (const auto &[key, value] : request_->getParameters()) {
        result[key] = value;
    }

    return result;
}

std::string LuaRequest::param(const std::string& key) const {
    // debug prints
    std::cerr << "Looking for parameter: " << key << "\n";
    for (const auto& [name, value] : request_->getParameters()) {
        std::cerr << "  " << name << " = " << value << "\n";
    }

    return request_->getParameter(key);
}