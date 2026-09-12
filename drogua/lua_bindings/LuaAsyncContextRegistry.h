#pragma once

#include <lua.hpp>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

class LuaAsyncContextRegistry {
public:
    template<typename T>
    static void set(lua_State* L, const std::shared_ptr<T>& context) {
        if (!L) {
            throw std::invalid_argument("LuaAsyncContextRegistry::set received null Lua state");
        }

        if (!context) {
            throw std::invalid_argument("LuaAsyncContextRegistry::set received null context");
        }

        std::lock_guard<std::mutex> lock(mutex<T>());
        contexts<T>()[L] = context;
    }

    template<typename T>
    static std::shared_ptr<T> get(lua_State* L) {
        if (!L) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(mutex<T>());

        auto it = contexts<T>().find(L);

        if (it == contexts<T>().end()) {
            return nullptr;
        }

        return it->second;
    }

    template<typename T>
    static void clear(lua_State* L) {
        if (!L) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex<T>());
        contexts<T>().erase(L);
    }

private:
    template<typename T>
    static std::mutex& mutex() {
        static std::mutex value;
        return value;
    }

    template<typename T>
    static std::unordered_map<lua_State*, std::shared_ptr<T>>& contexts() {
        static std::unordered_map<lua_State*, std::shared_ptr<T>> value;
        return value;
    }
};
