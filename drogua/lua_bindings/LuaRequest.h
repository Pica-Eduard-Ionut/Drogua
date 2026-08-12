#pragma once

#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

class LuaRequest {
    public:
        explicit LuaRequest(const drogon::HttpRequestPtr& request);

        // HTTP request metadata
        std::string method() const;
        std::string path() const;
        bool secure() const;
        std::string ip() const;
        
        // Query parameters
        std::string query(const std::string& key) const;
        luabridge::LuaRef queryParams(lua_State* L) const;

        // Headers
        std::string header(const std::string& name) const;
        luabridge::LuaRef headers(lua_State* L) const;

        // Cookies
        std::string cookie(const std::string& name) const;
        luabridge::LuaRef cookies(lua_State* L) const;

        // Request body
        std::string body() const;

        // Parsed JSON request body.
        // Returns nil if the request does not contain valid JSON.
        luabridge::LuaRef json(lua_State* L) const;

    private:
        static luabridge::LuaRef jsonToLua(lua_State* L, const Json::Value& value);
        drogon::HttpRequestPtr request_;
};