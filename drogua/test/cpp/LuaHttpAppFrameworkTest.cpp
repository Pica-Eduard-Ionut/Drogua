#define DROGON_TEST_MAIN

#include <drogon/drogon_test.h>

#include <lua_bindings/LuaHttpAppFramework.h>

DROGON_TEST(LuaHttpAppFrameworkInstance) {
    auto& app = LuaHttpAppFramework::instance();
    CHECK(&app == &LuaHttpAppFramework::instance());
}
