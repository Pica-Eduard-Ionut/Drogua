#include <drogon/drogon_test.h>
#include <drogon/drogon.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <lua_bindings/LuaResponse.h>

#include <string>
#include <sstream>
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

// == BASIC TESTS ==
// Verifies that a newly created response has Drogon's default status.
DROGON_TEST(LuaResponseDefaultStatus) {
    LuaResponse response;

    CHECK(response.status() == 200);
}

// Verifies that the response status can be changed.
DROGON_TEST(LuaResponseStatus) {
    LuaResponse response;
    response.setStatus(201);

    CHECK(response.status() == 201);
}

// Verifies that the response body can be set and retrieved.
DROGON_TEST(LuaResponseBody) {
    LuaResponse response;
    response.setBody("hello");

    CHECK(response.body() == "hello");
}

// Verifies that a response body can be replaced.
DROGON_TEST(LuaResponseBodyReplace) {
    LuaResponse response;
    response.setBody("first");
    response.setBody("second");

    CHECK(response.body() == "second");
}

// == BODY TESTS ==
// Verifies that an individual response header can be set and retrieved.
DROGON_TEST(LuaResponseHeader) {
    LuaResponse response;
    response.setHeader("X-Test", "hello");

    CHECK(response.header("X-Test") == "hello");
}

// Verifies that response headers are converted to a Lua table.
DROGON_TEST(LuaResponseHeaders) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    LuaResponse response;
    response.setHeader("x-test", "hello");
    response.setHeader("x-another", "world");
    auto headers = response.headers(L);

    REQUIRE(headers.isTable());
    CHECK(headers["x-test"].cast<std::string>().value() == "hello");
    CHECK(headers["x-another"].cast<std::string>().value() == "world");
}

// == CONTENT TYPE TESTS ==
DROGON_TEST(LuaResponseContentType) {
    LuaResponse response;
    response.setContentType("application/json");

    CHECK(response.contentType() == "application/json");
}

// == JSON TESTS ==
// Verifies that a Lua table can be converted into a JSON object.
DROGON_TEST(LuaResponseJson) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    LuaResponse response;
    auto json = luabridge::newTable(L);
    json["name"] = "Drogua";
    json["age"] = 42;
    json["active"] = true;
    response.json(L, json);

    CHECK(response.contentType() == "application/json");

    Json::Value parsed;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(response.body());

    REQUIRE(Json::parseFromStream(
        reader,
        stream,
        &parsed,
        &errors
    ));

    CHECK(parsed["name"].asString() == "Drogua");
    CHECK(parsed["age"].asInt() == 42);
    CHECK(parsed["active"].asBool() == true);
}

// Verifies that nested Lua tables are converted into nested JSON objects.
DROGON_TEST(LuaResponseJsonNested) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    LuaResponse response;
    auto json = luabridge::newTable(L);
    json["message"] = "Hello";
    auto user = luabridge::newTable(L);
    user["id"] = 123;
    user["name"] = "Marian";
    user["active"] = true;
    json["user"] = user;

    response.json(L, json);
    Json::Value parsed;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(response.body());
    REQUIRE(Json::parseFromStream(
        reader,
        stream,
        &parsed,
        &errors
    ));

    CHECK(parsed["message"].asString() == "Hello");
    REQUIRE(parsed["user"].isObject());
    CHECK(parsed["user"]["id"].asInt() == 123);
    CHECK(parsed["user"]["name"].asString() == "Marian");
    CHECK(parsed["user"]["active"].asBool() == true);
}

// Verifies that a sequential Lua table becomes a JSON array.
DROGON_TEST(LuaResponseJsonArray) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    LuaResponse response;
    auto json = luabridge::newTable(L);
    auto items = luabridge::newTable(L);
    items[1] = "one";
    items[2] = "two";
    items[3] = "three";
    json["items"] = items;

    response.json(L, json);
    Json::Value parsed;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(response.body());
    REQUIRE(Json::parseFromStream(
        reader,
        stream,
        &parsed,
        &errors
    ));

    REQUIRE(parsed["items"].isArray());
    REQUIRE(parsed["items"].size() == 3);
    CHECK(parsed["items"][0].asString() == "one");
    CHECK(parsed["items"][1].asString() == "two");
    CHECK(parsed["items"][2].asString() == "three");
}

// Verifies conversion of a realistic nested Lua response structure.
DROGON_TEST(LuaResponseJsonComplex) {
    auto* L = createLuaState();
    REQUIRE(L != nullptr);

    LuaResponse response;
    auto json = luabridge::newTable(L);
    json["ok"] = true;
    json["message"] = "Request successful";
    json["count"] = 2;
    auto users = luabridge::newTable(L);

    auto user1 = luabridge::newTable(L);
    user1["id"] = 1;
    user1["name"] = "Marian";

    auto user2 = luabridge::newTable(L);
    user2["id"] = 2;
    user2["name"] = "Iulia";
    users[1] = user1;
    users[2] = user2;

    json["users"] = users;
    response.json(L, json);
    Json::Value parsed;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(response.body());

    REQUIRE(Json::parseFromStream(
        reader,
        stream,
        &parsed,
        &errors
    ));

    CHECK(parsed["ok"].asBool() == true);
    CHECK(parsed["message"].asString() == "Request successful");
    CHECK(parsed["count"].asInt() == 2);

    REQUIRE(parsed["users"].isArray());
    REQUIRE(parsed["users"].size() == 2);

    CHECK(parsed["users"][0]["id"].asInt() == 1);
    CHECK(parsed["users"][0]["name"].asString() == "Marian");

    CHECK(parsed["users"][1]["id"].asInt() == 2);
    CHECK(parsed["users"][1]["name"].asString() == "Iulia");
}

// == DRAGON RESPONSE TEST ==
// Verifies that the underlying Drogon response is accessible.
DROGON_TEST(LuaResponseUnderlyingResponse) {
    LuaResponse response;

    response.setStatus(201);
    response.setBody("created");
    response.setHeader("x-test", "hello");
    auto underlying = response.response();

    REQUIRE(underlying != nullptr);
    CHECK(static_cast<int>(underlying->statusCode()) == 201);
    CHECK(std::string(underlying->body()) == "created");
    CHECK(underlying->getHeader("x-test") == "hello");
}