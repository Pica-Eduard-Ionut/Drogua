#include <drogon/drogon_test.h>
#include <lua_bindings/LuaHttpAppFramework.h>
#include <stdexcept>

DROGON_TEST(LuaHttpAppFrameworkInstance) {
    auto& app1 = LuaHttpAppFramework::instance();
    auto& app2 = LuaHttpAppFramework::instance();
    CHECK(&app1 == &app2);
}

DROGON_TEST(LuaHttpAppFrameworkSetThreadNum) {
    auto& app = LuaHttpAppFramework::instance();
    auto& result = app.setThreadNum(2);
    CHECK(&result == &app);
    CHECK(drogon::app().getThreadNum() == 2);
}

DROGON_TEST(LuaHttpAppFrameworkAddListener) {
    auto& app = LuaHttpAppFramework::instance();
    auto& result = app.addListener("127.0.0.1", 8080);
    CHECK(&result == &app);
}

DROGON_TEST(LuaHttpAppFrameworkSetLogPath) {
    auto& app = LuaHttpAppFramework::instance();
    auto& result = app.setLogPath("./");
    CHECK(&result == &app);
}

DROGON_TEST(LuaHttpAppFrameworkSetLogLevel) {
    auto& app = LuaHttpAppFramework::instance();
    CHECK_NOTHROW(app.setLogLevel("TRACE"));
    CHECK_NOTHROW(app.setLogLevel("DEBUG"));
    CHECK_NOTHROW(app.setLogLevel("INFO"));
    CHECK_NOTHROW(app.setLogLevel("WARN"));
}

DROGON_TEST(LuaHttpAppFrameworkSetLogLevelInvalid) {
    auto& app = LuaHttpAppFramework::instance();
    CHECK_THROWS_AS(
        app.setLogLevel("INVALID"),
        std::invalid_argument
    );
}

DROGON_TEST(LuaHttpAppFrameworkEnableRunAsDaemon){
    auto& app = LuaHttpAppFramework::instance();
    auto& result = app.enableRunAsDaemon();
    CHECK(&result == &app);
}

DROGON_TEST(LuaHttpAppFrameworkLoadJsonConfig) {
    auto& app = LuaHttpAppFramework::instance();
    auto& result = app.loadJsonConfig("config");
    CHECK(&result == &app);
}