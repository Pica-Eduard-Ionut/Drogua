#include "LuaRequest.h"

LuaRequest::LuaRequest(const drogon::HttpRequestPtr& request)
    : request_(request) {}

// Request metadata
std::string LuaRequest::method() const {
    return drogon::to_string(request_->getMethod());
}

std::string LuaRequest::path() const {
    return request_->getPath();
}

bool LuaRequest::secure() const {
    return request_->isOnSecureConnection();
}

std::string LuaRequest::ip() const {
    return request_->getPeerAddr().toIp();
}

// Query parameters
std::string LuaRequest::query(const std::string& key) const {
    return request_->getParameter(key);
}

luabridge::LuaRef LuaRequest::queryParams(lua_State* L) const {
    auto result = luabridge::newTable(L);
    for (const auto& [key, value] : request_->getParameters()) {
        result[key] = value;
    }
    return result;
}

// Headers
std::string LuaRequest::header(const std::string& name) const {
    return request_->getHeader(name);
}

luabridge::LuaRef LuaRequest::headers(lua_State* L) const {
    auto result = luabridge::newTable(L);
    for (const auto& [name, value] : request_->headers()) {
        result[name] = value;
    }
    return result;
}

// Cookies
std::string LuaRequest::cookie(const std::string& name) const {
    return request_->getCookie(name);
}

luabridge::LuaRef LuaRequest::cookies(lua_State* L) const {
    auto result = luabridge::newTable(L);
    for (const auto& [name, value] : request_->cookies()) {
        result[name] = value;
    }
    return result;
}

// Body
std::string LuaRequest::body() const {
    return std::string(request_->getBody());
}

// JSON
luabridge::LuaRef LuaRequest::json(lua_State* L) const {
    auto jsonObject = request_->getJsonObject();
    if (!jsonObject) {
        return luabridge::LuaRef(L);
    }
    return jsonToLua(L, *jsonObject);
}

// Json::Value -> Lua value
luabridge::LuaRef LuaRequest::jsonToLua(lua_State* L, const Json::Value& value) {
    switch (value.type()) {
        case Json::nullValue:
            return luabridge::LuaRef(L);

        case Json::booleanValue:
            return luabridge::LuaRef(L, value.asBool());

        case Json::intValue:
        case Json::uintValue:
            return luabridge::LuaRef(L, static_cast<lua_Integer>(value.asLargestInt()));

        case Json::realValue:
            return luabridge::LuaRef(L, value.asDouble());

        case Json::stringValue:
            return luabridge::LuaRef(L, value.asString());

        case Json::arrayValue: {
            auto table = luabridge::newTable(L);
            for (Json::ArrayIndex i = 0; i < value.size(); ++i) {
                table[i + 1] = jsonToLua(L, value[i]);
            }
            return table;
        }

        case Json::objectValue: {
            auto table = luabridge::newTable(L);
            for (const auto& key : value.getMemberNames()) {
                table[key] = jsonToLua(L, value[key]);
            }
            return table;
        }

        default:
            return luabridge::LuaRef(L);
    }
}