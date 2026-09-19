#include "LuaHttpResult.h"

LuaHttpResult::LuaHttpResult(const drogon::HttpResponsePtr& response) 
    : response_(response) {
    if (!response_) {
        throw std::runtime_error("LuaHttpResult requires non-null HttpResponse");
    }
}

int LuaHttpResult::status() const { 
    return static_cast<int>(response_->statusCode()); 
}

std::string LuaHttpResult::statusMessage() const {
    // Drogon doesn't have getStatusMessage() - return status code as string
    return std::to_string(static_cast<int>(response_->statusCode()));
}

std::string LuaHttpResult::body() const { 
    return std::string(response_->getBody()); 
}

luabridge::LuaRef LuaHttpResult::headers(lua_State* L) const {
    luabridge::LuaRef table = luabridge::newTable(L);
    const auto& headers = response_->getHeaders();
    for (const auto& [k, v] : headers) {
        table[k] = v;
    }
    return table;
}

luabridge::LuaRef LuaHttpResult::json(lua_State* L) const {
    if (cachedJson_.has_value()) {
        return cachedJson_.value();
    }
    
    auto ct = response_->getContentType();
    if (ct != drogon::ContentType::CT_APPLICATION_JSON) {
        return luabridge::LuaRef(L); // Return nil if not JSON
    }
    
    try {
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        Json::Value json;
        std::string errs;
    
        std::string bodyStr(response_->getBody());
        if (reader->parse(bodyStr.data(), bodyStr.data() + bodyStr.size(), &json, &errs)) {
            cachedJson_ = jsonToLuaValue(L, json);
        }

    } catch (...) {
        return luabridge::LuaRef(L);
    }
    
    return cachedJson_.value_or(luabridge::LuaRef(L));
}

bool LuaHttpResult::ok() const {
    auto code = response_->statusCode();
    return code >= drogon::HttpStatusCode::k200OK && code < drogon::HttpStatusCode::k300MultipleChoices;
}

luabridge::LuaRef LuaHttpResult::toTable(lua_State* L) const {
    luabridge::LuaRef table = luabridge::newTable(L);
    
    table["status"] = status();
    table["statusMessage"] = statusMessage();
    table["body"] = body();
    table["headers"] = headers(L);
    table["ok"] = ok();
    
    return table;
}

luabridge::LuaRef LuaHttpResult::jsonToLuaValue(lua_State* L, const Json::Value& json) {
    if (json.isNull()) {
        return luabridge::LuaRef(L); // Returns nil
    }
    if (json.isBool()) {
        return luabridge::LuaRef(L, json.asBool());
    }
    if (json.isInt64()) {
        return luabridge::LuaRef(L, static_cast<lua_Integer>(json.asInt64()));
    }
    if (json.isUInt64()) {
        return luabridge::LuaRef(L, static_cast<lua_Integer>(json.asUInt64()));
    }
    if (json.isDouble()) {
        return luabridge::LuaRef(L, json.asDouble());
    }
    if (json.isString()) {
        return luabridge::LuaRef(L, json.asString());
    }
    if (json.isArray()) {
        luabridge::LuaRef table = luabridge::newTable(L);
        for (Json::ArrayIndex i = 0; i < json.size(); ++i) {
            // Lua arrays are 1-indexed
            table[static_cast<lua_Integer>(i + 1)] = jsonToLuaValue(L, json[i]);
        }
        return table;
    }
    if (json.isObject()) {
        luabridge::LuaRef table = luabridge::newTable(L);
        for (const auto& key : json.getMemberNames()) {
            table[key] = jsonToLuaValue(L, json[key]);
        }
        return table;
    }
    return luabridge::LuaRef(L);
}