#pragma once

#include "LuaMiddleware.h"

#include <drogon/drogon.h>
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class LuaMiddlewareManager {
public:
    using MiddlewareChain = std::vector<LuaMiddleware*>;

    static LuaMiddlewareManager& instance();

    /*
     * Lua-facing factory.
     *
     * Lua:
     *
     *   local auth = Drogua.Middleware.create(function(...)
     *   end)
     */
    static int luaCreate(lua_State* L);

    /*
     * Creates and owns the middleware.
     */
    LuaMiddleware* create(lua_State* L, const luabridge::LuaRef& function);

    void add(drogon::HttpMethod method, const std::string& path, const luabridge::LuaRef& middlewareTable);

    const MiddlewareChain* get(drogon::HttpMethod method, const std::string& path) const;

    bool has(drogon::HttpMethod method, const std::string& path) const;

    void remove(drogon::HttpMethod method, const std::string& path);

    void clear();

private:
    LuaMiddlewareManager() = default;

    struct RouteKey {
        drogon::HttpMethod method;
        std::string path;

        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };

    struct RouteKeyHash {
        std::size_t operator()(const RouteKey& key) const;
    };

    static MiddlewareChain luaTableToChain(const luabridge::LuaRef& table);

    std::vector<std::unique_ptr<LuaMiddleware>> middlewares_;

    std::unordered_map<RouteKey, MiddlewareChain, RouteKeyHash> routes_;
};
