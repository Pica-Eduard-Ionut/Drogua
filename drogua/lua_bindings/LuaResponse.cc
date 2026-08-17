#include "LuaResponse.h"

LuaResponse::LuaResponse()
    : response_(drogon::HttpResponse::newHttpResponse()),
      contentType_() {}

// Status
int LuaResponse::status() const {
    return static_cast<int>(response_->statusCode());
}

LuaResponse& LuaResponse::setStatus(int code) {
    response_->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
    return *this;
}

// Body
std::string LuaResponse::body() const {
    return std::string(response_->body());
}

LuaResponse& LuaResponse::setBody(const std::string& body) {
    response_->setBody(body);
    return *this;
}

// Headers
std::string LuaResponse::header(const std::string& name) const {
    return response_->getHeader(name);
}

LuaResponse& LuaResponse::setHeader(const std::string& name, const std::string& value) {
    response_->addHeader(name, value);
    return *this;
}

luabridge::LuaRef LuaResponse::headers(lua_State* L) const {
    auto result = luabridge::newTable(L);
    for (const auto& [name, value] : response_->headers())
        result[name] = value;

    return result;
}

// Content-Type
std::string LuaResponse::contentType() const {
    return contentType_;
}

LuaResponse& LuaResponse::setContentType(const std::string& type) {
    response_->setContentTypeString(type);
    contentType_ = type;

    return *this;
}

// JSON
LuaResponse& LuaResponse::json(lua_State* L, const luabridge::LuaRef& value) {
    setContentType("application/json");
    response_->setBody(luaToJson(L, value).toStyledString());

    return *this;
}

// Lua -> JSON
Json::Value LuaResponse::luaToJson(lua_State* L, const luabridge::LuaRef& value) {
    value.push(L);
    auto result = luaValueToJson(L, -1);
    lua_pop(L, 1);
    return result;
}

Json::Value LuaResponse::luaValueToJson(lua_State* L, int index) {
    if (index < 0)
        index = lua_gettop(L) + index + 1;

    switch (lua_type(L, index)) {
        case LUA_TNIL:
            return Json::nullValue;

        case LUA_TBOOLEAN:
            return lua_toboolean(L, index) != 0;

        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
                return static_cast<Json::Int64>(lua_tointeger(L, index));

            return lua_tonumber(L, index);

        case LUA_TSTRING:
            return lua_tostring(L, index);

        case LUA_TTABLE: {
            bool isArray = true;
            lua_Integer expected = 1;
            // Check whether the table is a sequential array.
            lua_pushnil(L);
            while (lua_next(L, index) != 0) {
                if (!lua_isinteger(L, -2) ||
                    lua_tointeger(L, -2) != expected) {
                    isArray = false;
                }

                if (lua_isinteger(L, -2)) {
                    auto key = lua_tointeger(L, -2);
                    if (key >= expected)
                        expected = key + 1;
                }

                lua_pop(L, 1);
            }

            if (isArray) {
                Json::Value result(Json::arrayValue);
                const auto length = static_cast<lua_Integer>(lua_rawlen(L, index));
                for (lua_Integer i = 1; i <= length; ++i) {
                    lua_rawgeti(L, index, i);
                    result.append(luaValueToJson(L, -1));
                    lua_pop(L, 1);
                }

                return result;
            }

            Json::Value result(Json::objectValue);
            // Convert string-keyed entries into an object.
            lua_pushnil(L);
            while (lua_next(L, index) != 0) {
                if (lua_isstring(L, -2))
                    result[lua_tostring(L, -2)] = luaValueToJson(L, -1);

                lua_pop(L, 1);
            }

            return result;
        }

        default:
            throw std::runtime_error("Cannot convert Lua value to JSON");
    }
}

// Underlying Drogon response
drogon::HttpResponsePtr LuaResponse::response() const {
    return response_;
}