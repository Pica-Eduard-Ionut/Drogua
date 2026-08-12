#include "LuaRoutes.h"
#include "LuaRequest.h"

#include <stdexcept>
#include <string>

using namespace drogon;

void LuaRoutes::get(const std::string &path, const luabridge::LuaRef &handler) {
    registerRoute(path, Get, handler);
}

void LuaRoutes::post(const std::string &path, const luabridge::LuaRef &handler) {
    registerRoute(path, Post, handler);
}

void LuaRoutes::put(const std::string &path, const luabridge::LuaRef &handler) {
    registerRoute(path, Put, handler);
}

void LuaRoutes::del(const std::string &path, const luabridge::LuaRef &handler) {
    registerRoute(path, Delete, handler);
}

void LuaRoutes::patch(const std::string &path, const luabridge::LuaRef &handler) {
    registerRoute(path, Patch, handler);
}

Json::Value LuaRoutes::luaTableToJson(const luabridge::LuaRef &table) {
    Json::Value result;
    if (!table.isTable())
        return result;

    lua_State *L = table.state();
    table.push(L);
    const int tableIndex = lua_gettop(L);
    // Push first key
    lua_pushnil(L);
    while (lua_next(L, tableIndex) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING) {
            const char *key = lua_tostring(L, -2);
            switch (lua_type(L, -1)) {
            case LUA_TSTRING:
                result[key] = lua_tostring(L, -1);
                break;

            case LUA_TNUMBER:
                // Lua integer
                if (lua_isinteger(L, -1))
                    result[key] = static_cast<Json::Int64>(lua_tointeger(L, -1));
                // Lua floating-point number
                else
                    result[key] = lua_tonumber(L, -1);
                break;

            case LUA_TBOOLEAN:
                result[key] = lua_toboolean(L, -1) != 0;
                break;

            case LUA_TNIL:
                result[key] = Json::nullValue;
                break;

            case LUA_TTABLE: {
                luabridge::LuaRef nested = luabridge::LuaRef::fromStack(L, -1);

                result[key] = luaTableToJson(nested);
                break;
            }

            default:
                std::cerr << "Unsupported Lua value for key '" << key << "'\n";
                break;
            }
        }
        // Remove value, keep key for lua_next()
        lua_pop(L, 1);
    }
    // Remove the table
    lua_pop(L, 1);
    return result;
}

void LuaRoutes::registerRoute(const std::string &path, drogon::HttpMethod method, const luabridge::LuaRef &handler) {
    if (!handler.isTable() && !handler.isFunction())
        throw std::runtime_error("Route requires a Lua table or function");

    app().registerHandler(path, [handler](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        std::cerr << "PATH: " << req->path() << '\n';

        for (const auto& [key, value] : req->getParameters()) {
            std::cerr << "PARAM: " << key << " = " << value << '\n';
        }
            try {
                Json::Value json = LuaRoutes::executeHandler(handler, req);
                LuaRoutes::sendJsonResponse(json, std::move(callback));
            }
            catch (const std::exception &e) {
                LuaRoutes::sendErrorResponse(e.what(), std::move(callback));
            }
    },
    {method});
}

Json::Value LuaRoutes::executeHandler(const luabridge::LuaRef &handler, const drogon::HttpRequestPtr &req) {
    if (handler.isTable())
        return executeLuaTable(handler);

    if (handler.isFunction())
        return executeLuaFunction(handler, req);

    throw std::runtime_error("Invalid Lua route handler");
}

Json::Value LuaRoutes::executeLuaTable(const luabridge::LuaRef &handler) {
    return luaTableToJson(handler);
}

Json::Value LuaRoutes::executeLuaFunction(const luabridge::LuaRef& handler, const drogon::HttpRequestPtr& req) {
    lua_State* L = handler.state();
    const int base = lua_gettop(L);

    // Wrap Drogon request for Lua.
    LuaRequest luaRequest(req);

    // Push Lua function.
    handler.push(L);

    // Push request wrapper as the first argument.
    auto pushResult =
    luabridge::Stack<LuaRequest*>::push(L, &luaRequest);

    if (!pushResult) {
        lua_settop(L, base);

        throw std::runtime_error(
            "Failed to push LuaRequest: " +
            pushResult.message());
    }

    // Call Lua function(req), expecting one return value.
    const int status = lua_pcall(L, 1, 1, 0);

    if (status != LUA_OK) {
        const char* error = lua_tostring(L, -1);
        std::string message = error ? error : "Unknown Lua error";
        lua_settop(L, base);
        throw std::runtime_error("Lua route handler failed: " + message);
    }

    // Get the Lua return value.
    auto result = luabridge::Stack<luabridge::LuaRef>::get(L, -1);
    if (!result) {
        lua_settop(L, base);

        throw std::runtime_error("Failed to retrieve Lua route result: " + result.message());
    }

    luabridge::LuaRef luaResult = result.value();

    // Restore the Lua stack.
    lua_settop(L, base);

    if (!luaResult.isTable()) {
        throw std::runtime_error("Lua route handler must return a table");
    }

    return luaTableToJson(luaResult);
}

void LuaRoutes::sendJsonResponse(const Json::Value &json, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto response = drogon::HttpResponse::newHttpJsonResponse(json);
    callback(response);
}

void LuaRoutes::sendErrorResponse(const std::string &message, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Json::Value error;
    error["error"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(error);
    response->setStatusCode(drogon::k500InternalServerError);
    callback(response);
}
