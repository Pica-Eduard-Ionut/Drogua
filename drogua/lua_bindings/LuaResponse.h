#pragma once

#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

class LuaResponse {
    public:
        LuaResponse();

        // Status
        int status() const;
        LuaResponse& setStatus(int code);

        // Body
        std::string body() const;
        LuaResponse& setBody(const std::string& body);

        // Headers
        std::string header(const std::string& name) const;
        LuaResponse& setHeader(const std::string& name, const std::string& value);

        luabridge::LuaRef headers(lua_State* L) const;

        // Content type
        std::string contentType() const;
        LuaResponse& setContentType(const std::string& type);

        // JSON
        LuaResponse& json(const luabridge::LuaRef& value);

        // Underlying Drogon response
        drogon::HttpResponsePtr response() const;

    private:
        static Json::Value luaToJson(const luabridge::LuaRef& value);
        static Json::Value luaValueToJson(lua_State* L, int index);
        drogon::HttpResponsePtr response_;
        std::string contentType_;
};