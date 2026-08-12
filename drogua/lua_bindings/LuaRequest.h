#pragma once

#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

class LuaRequest {
public:
    explicit LuaRequest(const drogon::HttpRequestPtr& request);

    // Get a single route parameter.
    std::string param(const std::string& key) const;

    // Get all route parameters as a Lua table.
    luabridge::LuaRef params(lua_State* L) const;

private:
    drogon::HttpRequestPtr request_;
};