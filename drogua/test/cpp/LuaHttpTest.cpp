#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaHttp.h>
#include <lua_bindings/LuaHttpResult.h>
#include <lua_bindings/LuaRoutes.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

using namespace drogon;

namespace {

// ---------------------------------------------------------
// Test server helpers
// ---------------------------------------------------------

std::string getLocalBaseUrl() {
    auto listeners = app().getListeners();

    if (!listeners.empty())
        return "http://" + listeners[0].toIpPort();

    return "http://127.0.0.1:8888";
}

trantor::EventLoop* getMainAppLoop() {
    auto* loop = app().getLoop();
    return loop;
}

void registerTestHttpRoutes() {
    app().registerHandler(
        "/test-http-get",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            Json::Value json;
            json["message"] = "hello world";
            json["status"] = "success";

            callback(HttpResponse::newHttpJsonResponse(json));
        },
        {Get});

    app().registerHandler(
        "/test-http-post",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            Json::Value json;
            auto jsonObj = req->getJsonObject();

            if (jsonObj)
                json["received_body"] = *jsonObj;
            else
                json["received_body"] = Json::nullValue;

            json["method"] = "POST";

            callback(HttpResponse::newHttpJsonResponse(json));
        },
        {Post});

    app().registerHandler(
        "/test-http-error",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Internal Server Error");

            callback(resp);
        },
        {Get});
}

bool waitForCompletion(std::mutex& mutex, std::condition_variable& cv, bool& completed, bool& failed) {
    std::unique_lock<std::mutex> lock(mutex);

    return cv.wait_for(lock, std::chrono::seconds(5), [&]() {
        return completed || failed;
    });
}

} // namespace

// =========================================================
// LuaHttp Async Tests
// =========================================================

DROGON_TEST(LuaHttpAsyncGet) {
    registerTestHttpRoutes();
    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);
    luaL_openlibs(L);

    std::string url = getLocalBaseUrl() + "/test-http-get";
    luabridge::LuaRef options = luabridge::newTable(L);

    options["method"] = "GET";
    auto* loop = getMainAppLoop();
    REQUIRE(loop != nullptr);

    std::mutex mutex;
    std::condition_variable cv;
    bool completed = false;
    bool failed = false;
    std::shared_ptr<LuaHttpResult> asyncResult;

    LuaHttp::requestAsync(
        url,
        options,
        loop,
        [&](std::shared_ptr<LuaHttpResult> result) {
            std::lock_guard<std::mutex> lock(mutex);

            completed = true;
            asyncResult = std::move(result);

            cv.notify_one();
        },
        [&](const std::string& error) {
            std::lock_guard<std::mutex> lock(mutex);

            failed = true;

            LOG_ERROR << "Async HTTP GET failed: " << error;

            cv.notify_one();
        },
        0,
        100);

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(completed);
    CHECK(!failed);

    REQUIRE(asyncResult != nullptr);

    CHECK(asyncResult->ok());
    CHECK(asyncResult->status() == 200);

    auto json = asyncResult->json(L);

    REQUIRE(json.isTable());

    CHECK(json["message"].cast<std::string>().value() == "hello world");

    
}

DROGON_TEST(LuaHttpAsyncPostWithTableBody) {
    registerTestHttpRoutes();

    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);

    std::string url = getLocalBaseUrl() + "/test-http-post";
    luabridge::LuaRef options = luabridge::newTable(L);

    options["method"] = "POST";

    luabridge::LuaRef headers = luabridge::newTable(L);
    headers["Content-Type"] = "application/json";
    options["headers"] = headers;

    options["body"] = R"({"item": "test_item", "qty": 5})";

    auto* loop = getMainAppLoop();

    REQUIRE(loop != nullptr);

    std::mutex mutex;
    std::condition_variable cv;
    bool completed = false;
    bool failed = false;
    std::shared_ptr<LuaHttpResult> asyncResult;

    LuaHttp::requestAsync(
        url,
        options,
        loop,
        [&](std::shared_ptr<LuaHttpResult> result) {
            std::lock_guard<std::mutex> lock(mutex);

            completed = true;
            asyncResult = std::move(result);

            cv.notify_one();
        },
        [&](const std::string& error) {
            std::lock_guard<std::mutex> lock(mutex);

            failed = true;

            LOG_ERROR << "Async HTTP POST failed: " << error;

            cv.notify_one();
        },
        0,
        100);

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(completed);
    CHECK(!failed);

    REQUIRE(asyncResult != nullptr);

    CHECK(asyncResult->ok());
    CHECK(asyncResult->status() == 200);

    auto json = asyncResult->json(L);

    REQUIRE(json.isTable());

    auto received = json["received_body"];

    REQUIRE(received.isTable());

    CHECK(received["item"].cast<std::string>().value() == "test_item");
    CHECK(received["qty"].cast<int>().value() == 5);

    
}

DROGON_TEST(LuaHttpAsyncErrorHandling) {
    registerTestHttpRoutes();

    lua_State* L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);

    std::string url = getLocalBaseUrl() + "/test-http-error";
    luabridge::LuaRef options = luabridge::newTable(L);

    auto* loop = getMainAppLoop();

    REQUIRE(loop != nullptr);

    std::mutex mutex;
    std::condition_variable cv;
    bool completed = false;
    bool failed = false;
    std::shared_ptr<LuaHttpResult> asyncResult;

    LuaHttp::requestAsync(
        url,
        options,
        loop,
        [&](std::shared_ptr<LuaHttpResult> result) {
            std::lock_guard<std::mutex> lock(mutex);

            completed = true;
            asyncResult = std::move(result);

            cv.notify_one();
        },
        [&](const std::string& error) {
            std::lock_guard<std::mutex> lock(mutex);

            failed = true;

            LOG_ERROR << "Async HTTP error test failed: " << error;

            cv.notify_one();
        },
        0,
        100);

    const bool finished = waitForCompletion(mutex, cv, completed, failed);

    REQUIRE(finished);

    CHECK(completed);
    CHECK(!failed);

    REQUIRE(asyncResult != nullptr);

    // HTTP 500 is still a successful HTTP transport operation.
    // Therefore the success callback is expected.
    CHECK(!asyncResult->ok());
    CHECK(asyncResult->status() == 500);

    
}