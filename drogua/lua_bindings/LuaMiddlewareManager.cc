#include "LuaMiddlewareManager.h"

#include <stdexcept>
#include <utility>

LuaMiddlewareManager& LuaMiddlewareManager::instance() {
    static LuaMiddlewareManager manager;
    return manager;
}

LuaMiddleware* LuaMiddlewareManager::create(lua_State* L, const luabridge::LuaRef& function) {
    if (L == nullptr) {
        throw std::runtime_error("Drogua.Middleware.create received a null lua_State");
    }

    if (!function.isFunction()) {
        throw std::runtime_error("Drogua.Middleware.create expects a function");
    }

    auto middleware = std::unique_ptr<LuaMiddleware>(new LuaMiddleware(L, function));
    LuaMiddleware* ptr = middleware.get();

    middlewares_.push_back(std::move(middleware));

    return ptr;
}

int LuaMiddlewareManager::luaCreate(lua_State* L) {
    try {
        if (L == nullptr) {
            return luaL_error(L, "Middleware.create received a null lua_State");
        }

        if (lua_gettop(L) != 1) {
            return luaL_error(L, "Middleware.create expects one argument");
        }

        if (!lua_isfunction(L, 1)) {
            return luaL_error(L, "Middleware.create expects a function");
        }

        auto function = luabridge::Stack<luabridge::LuaRef>::get(L, 1);

        if (!function) {
            return luaL_error(L, "Middleware.create failed to read function: %s", function.message().c_str());
        }

        LuaMiddleware* middleware = instance().create(L, function.value());

        if (middleware == nullptr) {
            return luaL_error(L, "Middleware.create returned null middleware");
        }

        /*
         * Push the LuaMiddleware* as Lua userdata.
         */
        auto result = luabridge::Stack<LuaMiddleware*>::push(L, middleware);

        if (!result) {
            return luaL_error(L, "Middleware.create failed to push middleware: %s", result.message().c_str());
        }

        return 1;
    } catch (const std::exception& e) {
        return luaL_error(L, "Middleware.create failed: %s", e.what());
    }
}

void LuaMiddlewareManager::add(
    drogon::HttpMethod method,
    const std::string& path,
    const luabridge::LuaRef& middlewareTable) {
    if (!middlewareTable.isTable()) {
        throw std::runtime_error("Drogua.Middleware.add expects a table");
    }

    MiddlewareChain chain = luaTableToChain(middlewareTable);
    RouteKey key{method, path};

    routes_[std::move(key)] = std::move(chain);
}

LuaMiddlewareManager::MiddlewareChain LuaMiddlewareManager::luaTableToChain(const luabridge::LuaRef& table) {
    MiddlewareChain chain;
    const lua_Integer length = table.length();

    chain.reserve(static_cast<std::size_t>(length));

    lua_State* L = table.state();

    for (lua_Integer i = 1; i <= length; ++i) {
        luabridge::LuaRef value = table[i];

        if (value.isNil()) {
            throw std::runtime_error("Middleware table contains nil at index " + std::to_string(i));
        }

        const int base = lua_gettop(L);
        value.push(L);

        auto middleware = luabridge::Stack<LuaMiddleware*>::get(L, -1);
        lua_settop(L, base);

        if (!middleware) {
            throw std::runtime_error("Invalid middleware at index " + std::to_string(i) + ": " + middleware.message());
        }

        if (middleware.value() == nullptr) {
            throw std::runtime_error("Middleware at index " + std::to_string(i) + " is null");
        }

        chain.push_back(middleware.value());
    }

    return chain;
}

const LuaMiddlewareManager::MiddlewareChain* LuaMiddlewareManager::get(
    drogon::HttpMethod method,
    const std::string& path) const {
    RouteKey key{method, path};
    auto it = routes_.find(key);

    if (it == routes_.end())
        return nullptr;

    return &it->second;
}

bool LuaMiddlewareManager::has(drogon::HttpMethod method, const std::string& path) const {
    RouteKey key{method, path};
    return routes_.find(key) != routes_.end();
}

void LuaMiddlewareManager::remove(drogon::HttpMethod method, const std::string& path) {
    RouteKey key{method, path};
    routes_.erase(key);
}

void LuaMiddlewareManager::clear() {
    routes_.clear();
    middlewares_.clear();
}

std::size_t LuaMiddlewareManager::RouteKeyHash::operator()(const RouteKey& key) const {
    const std::size_t methodHash = std::hash<int>{}(static_cast<int>(key.method));
    const std::size_t pathHash = std::hash<std::string>{}(key.path);

    return methodHash ^
           (pathHash + 0x9e3779b9 + (methodHash << 6) + (methodHash >> 2));
}
