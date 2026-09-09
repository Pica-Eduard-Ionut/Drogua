#include "LuaAsyncContextRegistry.h"

#include <mutex>
#include <unordered_map>

namespace {
    std::mutex mutex;
    std::unordered_map<lua_State*, std::shared_ptr<LuaAsyncContext>> contexts;
}

void LuaAsyncContextRegistry::set(lua_State* L, const std::shared_ptr<LuaAsyncContext>& context) {
    if (!L) {
        throw std::invalid_argument("LuaAsyncContextRegistry::set received null Lua state");
    }

    if (!context) {
        throw std::invalid_argument("LuaAsyncContextRegistry::set received null context");
    }

    std::lock_guard<std::mutex> lock(mutex);
    contexts[L] = context;
}

std::shared_ptr<LuaAsyncContext> LuaAsyncContextRegistry::get(lua_State* L) {
    if (!L) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex);
    auto it = contexts.find(L);

    if (it == contexts.end()) {
        return nullptr;
    }

    return it->second;
}

void LuaAsyncContextRegistry::clear(lua_State* L) {
    if (!L) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    contexts.erase(L);
}
