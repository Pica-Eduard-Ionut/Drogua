#pragma once

#include <lua.hpp>
#include <memory>
#include <string>

#include <LuaBridge/LuaBridge.h>

class LuaCoroutineManager {
public:
    struct Coroutine {
        lua_State *owner = nullptr;
        lua_State *thread = nullptr;
        int registryRef = LUA_NOREF;

        Coroutine() = default;

        ~Coroutine();

        Coroutine(const Coroutine &) = delete;
        Coroutine &operator=(const Coroutine &) = delete;

        Coroutine(Coroutine &&) = delete;
        Coroutine &operator=(Coroutine &&) = delete;
    };

    using Ptr = std::shared_ptr<Coroutine>;

    enum class Status {
        Finished,
        Yielded,
        Error
    };

    struct ResumeResult {
        Status status;
        std::string error;
        int nresults = 0;
    };

    // Create and retain a coroutine belonging to L.
    static Ptr create(lua_State* L);

    // Push a LuaBridge3 function onto the coroutine.
    //
    // After this call, the coroutine stack contains:
    //
    //     function
    //
    // Arguments can then be pushed directly onto coroutine->thread.
    static void pushFunction(const Ptr& coroutine, const luabridge::LuaRef& function);

    // Resume the coroutine.
    //
    // nargs = number of arguments currently on the coroutine
    // stack after the function.
    static ResumeResult resume(const Ptr& coroutine, int nargs = 0);

    // Access the underlying Lua coroutine.
    static lua_State* state(const Ptr& coroutine);

    // Get the Lua error from the top of the coroutine stack.
    static std::string getError(lua_State* L);

    // Clear a Lua stack.
    static void clearStack(lua_State* L);
};