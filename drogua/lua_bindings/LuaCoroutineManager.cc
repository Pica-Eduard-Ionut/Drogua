#include "LuaCoroutineManager.h"

#include <stdexcept>

LuaCoroutineManager::Coroutine::~Coroutine() {
    if (owner != nullptr &&
        registryRef != LUA_NOREF &&
        registryRef != LUA_REFNIL) {

        luaL_unref(owner, LUA_REGISTRYINDEX, registryRef);

        registryRef = LUA_NOREF;
    }

    owner = nullptr;
    thread = nullptr;
}

LuaCoroutineManager::Ptr LuaCoroutineManager::create(lua_State *L)
{
    if (L == nullptr) {
        throw std::invalid_argument("LuaCoroutineManager::create: null lua_State");
    }

    auto coroutine = std::make_shared<Coroutine>();
    coroutine->owner = L;

    coroutine->thread = lua_newthread(L);

    if (coroutine->thread == nullptr) {
        throw std::runtime_error("Failed to create Lua coroutine");
    }

    coroutine->registryRef = luaL_ref(L, LUA_REGISTRYINDEX);

    return coroutine;
}

void LuaCoroutineManager::pushFunction(const Ptr &coroutine, const luabridge::LuaRef &function) {
    if (!coroutine) {
        throw std::invalid_argument("LuaCoroutineManager::pushFunction: null coroutine");
    }

    if (coroutine->thread == nullptr) {
        throw std::runtime_error("LuaCoroutineManager::pushFunction: invalid coroutine");
    }

    if (function.isNil()) {
        throw std::invalid_argument("LuaCoroutineManager::pushFunction: nil function");
    }

    if (!function.isFunction()) {
        throw std::invalid_argument("LuaCoroutineManager::pushFunction: LuaRef is not a function");
    }

    /*
     * LuaRef belongs to the owner Lua state.
     *
     * Push the function onto the owner state and then move it
     * onto the coroutine stack. Both belong to the same Lua VM.
     */
    function.push(coroutine->owner);
    lua_xmove(coroutine->owner, coroutine->thread, 1);
}

lua_State *LuaCoroutineManager::state(const Ptr &coroutine) {
    if (!coroutine) {
        throw std::invalid_argument("LuaCoroutineManager::state: null coroutine");
    }

    if (coroutine->thread == nullptr) {
        throw std::runtime_error("LuaCoroutineManager::state: invalid coroutine");
    }

    return coroutine->thread;
}

LuaCoroutineManager::ResumeResult LuaCoroutineManager::resume(const Ptr &coroutine, int nargs) {
    if (!coroutine) {
        throw std::invalid_argument("LuaCoroutineManager::resume: null coroutine");
    }

    if (coroutine->thread == nullptr) {
        throw std::runtime_error("LuaCoroutineManager::resume: invalid coroutine");
    }

    if (nargs < 0) {
        throw std::invalid_argument("LuaCoroutineManager::resume: negative nargs");
    }

    int nresults = 0;
    const int status = lua_resume(coroutine->thread, nullptr, nargs, &nresults);

    if (status == LUA_OK) {
        return {Status::Finished, {}, nresults};
    }

    if (status == LUA_YIELD) {
        return {Status::Yielded, {}, nresults};
    }

    return {Status::Error, getError(coroutine->thread), nresults};
}

std::string LuaCoroutineManager::getError(lua_State *L) {
    if (L == nullptr) {
        return "Unknown Lua error";
    }

    const char *message = lua_tostring(L, -1);

    if (message != nullptr) {
        return message;
    }

    return "Unknown Lua error";
}

void LuaCoroutineManager::clearStack(lua_State* L) {
    if (!L)
        return;

    lua_settop(L, 0);
}
