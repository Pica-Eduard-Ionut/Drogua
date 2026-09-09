#pragma once

#include "LuaAsyncContext.h"

class LuaAsyncContextRegistry {
    public:
        // set
        static void set(lua_State* L, const std::shared_ptr<LuaAsyncContext>& context);
        // get
        static std::shared_ptr<LuaAsyncContext> get(lua_State* L);
        // clear
        static void clear(lua_State* L);
};
