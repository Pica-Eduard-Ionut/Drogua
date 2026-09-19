#pragma once

#include <drogon/HttpResponse.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <memory>
#include <string>
#include <optional>

class LuaHttpResult {
    public:
        explicit LuaHttpResult(const drogon::HttpResponsePtr& response);
        
        // Lua-facing getters
        int status() const;
        std::string statusMessage() const;
        std::string body() const;
        
        // Headers as Lua table
        luabridge::LuaRef headers(lua_State* L) const;
        
        // Convenience
        bool ok() const;
        luabridge::LuaRef toTable(lua_State* L) const;
        luabridge::LuaRef json(lua_State* L) const;
        
        // Internal use
        const drogon::HttpResponsePtr& response() const { return response_; }
        
    private:
        drogon::HttpResponsePtr response_;
        mutable std::optional<luabridge::LuaRef> cachedJson_;
        static luabridge::LuaRef jsonToLuaValue(lua_State* L, const Json::Value& json);
};
