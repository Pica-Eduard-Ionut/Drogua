#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <lua_bindings/LuaRequest.h>

#include <string>
#include <vector>

using namespace drogon;

namespace {
    std::vector<lua_State*>& luaStates() {
        static std::vector<lua_State*> states;
        return states;
    }

    lua_State* createLuaState() {
        auto* L = luaL_newstate();
        if (!L)
            return nullptr;

        luaL_openlibs(L);
        luaStates().push_back(L);

        return L;
    }
}

// Verifies that the HTTP method is exposed correctly.
DROGON_TEST(LuaRequestMethod) {
    auto request = HttpRequest::newHttpRequest();
    request->setMethod(Get);
    LuaRequest luaRequest(request);
    CHECK(luaRequest.method() == "GET");
}

// Verifies that the request path is exposed correctly.
DROGON_TEST(LuaRequestPath) {
    auto request = HttpRequest::newHttpRequest();
    request->setPath("/users/123");
    LuaRequest luaRequest(request);
    CHECK(luaRequest.path() == "/users/123");
}

// Verifies that a newly created non-secure request is reported as non-secure.
DROGON_TEST(LuaRequestSecure) {
    auto request = HttpRequest::newHttpRequest();
    LuaRequest luaRequest(request);
    CHECK(luaRequest.secure() == false);
}

// Verifies that the peer IP can be retrieved without throwing.
DROGON_TEST(LuaRequestIp) {
    auto request = HttpRequest::newHttpRequest();
    LuaRequest luaRequest(request);
    CHECK_NOTHROW(luaRequest.ip());
}

// Verifies that an individual query parameter can be retrieved.
DROGON_TEST(LuaRequestQuery) {
    auto request = HttpRequest::newHttpRequest();
    request->setParameter("name", "Drogua");
    LuaRequest luaRequest(request);
    CHECK(luaRequest.query("name") == "Drogua");
}

// Verifies that query parameters are converted to a Lua table.
DROGON_TEST(LuaRequestQueryParams) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    request->setParameter("name", "Drogua");
    request->setParameter("id", "42");
    LuaRequest luaRequest(request);
    auto params = luaRequest.queryParams(L);

    REQUIRE(params.isTable());
    CHECK(params["name"].cast<std::string>().value() == "Drogua");
    CHECK(params["id"].cast<std::string>().value() == "42");
}

// Verifies that an individual header can be retrieved.
DROGON_TEST(LuaRequestHeader) {
    auto request = HttpRequest::newHttpRequest();
    request->addHeader("X-Test", "hello");
    LuaRequest luaRequest(request);
    CHECK(luaRequest.header("X-Test") == "hello");
}

// Verifies that request headers are converted to a Lua table.
DROGON_TEST(LuaRequestHeaders) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    request->addHeader("X-Test", "hello");
    LuaRequest luaRequest(request);
    auto headers = luaRequest.headers(L);

    REQUIRE(headers.isTable());
    CHECK(headers["x-test"].cast<std::string>().value() == "hello");
}

// Verifies that an individual cookie can be retrieved.
DROGON_TEST(LuaRequestCookie) {
    auto request = HttpRequest::newHttpRequest();
    request->addCookie("session", "abc123");
    LuaRequest luaRequest(request);

    CHECK(luaRequest.cookie("session") == "abc123");
}

// Verifies that request cookies are converted to a Lua table.
DROGON_TEST(LuaRequestCookies) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    request->addCookie("session", "abc123");
    LuaRequest luaRequest(request);
    auto cookies = luaRequest.cookies(L);

    REQUIRE(cookies.isTable());
    CHECK(cookies["session"].cast<std::string>().value() == "abc123");
}

// Verifies that the request body is exposed correctly.
DROGON_TEST(LuaRequestBody) {
    auto request = HttpRequest::newHttpRequest();
    request->setBody("hello");
    LuaRequest luaRequest(request);

    CHECK(luaRequest.body() == "hello");
}

// Verifies that a JSON object is converted to a Lua table
// with the expected primitive value types.
DROGON_TEST(LuaRequestJson) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    request->addHeader("Content-Type", "application/json");
    request->setBody(R"({"name":"Drogua","age":42,"active":true})");
    LuaRequest luaRequest(request);
    auto json = luaRequest.json(L);

    REQUIRE(json.isTable());
    CHECK(json["name"].cast<std::string>().value() == "Drogua");
    CHECK(json["age"].cast<int>().value() == 42);
    CHECK(json["active"].cast<bool>().value() == true);
}

// Verifies that JSON arrays are converted to Lua tables
// using Lua's 1-based array indexing.
DROGON_TEST(LuaRequestJsonArray) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    request->addHeader("Content-Type", "application/json");
    request->setBody(R"({"items":["one","two","three"]})");
    LuaRequest luaRequest(request);
    auto json = luaRequest.json(L);

    REQUIRE(json.isTable());
    REQUIRE(json["items"].isTable());
    CHECK(json["items"][1].cast<std::string>().value() == "one");
    CHECK(json["items"][2].cast<std::string>().value() == "two");
    CHECK(json["items"][3].cast<std::string>().value() == "three");
}

// Verifies that a request without JSON returns a nil Lua value.
DROGON_TEST(LuaRequestJsonMissing) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    auto request = HttpRequest::newHttpRequest();
    LuaRequest luaRequest(request);
    auto json = luaRequest.json(L);

    CHECK(json.isNil());
}