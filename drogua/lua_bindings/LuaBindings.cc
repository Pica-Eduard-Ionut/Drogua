#include "LuaBindings.h"

#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaRoutes.h>
#include <lua_bindings/LuaRequest.h>
#include <lua_bindings/LuaResponse.h>
#include <lua_bindings/LuaHttpAppFramework.h>

#include <iostream>
#include <string>

void draguaPrint(const std::string& message) {
    std::cout << "[Drogua] " << message << std::endl;
}

void registerDrogua(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("Drogua")

            .addFunction("print", &draguaPrint)

            .addFunction("app", &LuaHttpAppFramework::instance)
                .beginClass<LuaHttpAppFramework>("HttpAppFramework")
                    .addFunction("loadJsonConfig", &LuaHttpAppFramework::loadJsonConfig)
                    .addFunction("setThreadNum", &LuaHttpAppFramework::setThreadNum)
                    .addFunction("addListener", &LuaHttpAppFramework::addListener)
                    .addFunction("setLogPath", &LuaHttpAppFramework::setLogPath)
                    .addFunction("setLogLevel", &LuaHttpAppFramework::setLogLevel)
                    .addFunction("enableRunAsDaemon", &LuaHttpAppFramework::enableRunAsDaemon)
                    .addFunction("run", &LuaHttpAppFramework::run)
                .endClass()

            .beginNamespace("Routes")
                .addFunction("get", &LuaRoutes::get)
                .addFunction("post", &LuaRoutes::post)
                .addFunction("put", &LuaRoutes::put)
                .addFunction("delete", &LuaRoutes::del)
                .addFunction("patch", &LuaRoutes::patch)
            .endNamespace()

            .beginClass<LuaRequest>("Request")
                .addFunction("method", &LuaRequest::method)
                .addFunction("path", &LuaRequest::path)
                .addFunction("secure", &LuaRequest::secure)
                .addFunction("ip", &LuaRequest::ip)
                .addFunction("query", &LuaRequest::query)
                .addFunction("queryParams", &LuaRequest::queryParams)
                .addFunction("header", &LuaRequest::header)
                .addFunction("headers", &LuaRequest::headers)
                .addFunction("cookie", &LuaRequest::cookie)
                .addFunction("cookies", &LuaRequest::cookies)
                .addFunction("body", &LuaRequest::body)
                .addFunction("json", &LuaRequest::json)
            .endClass()

            .beginClass<LuaResponse>("Response")
                .addConstructor<void(*)()>()
                .addFunction("status", &LuaResponse::status)
                .addFunction("setStatus", &LuaResponse::setStatus)
                .addFunction("body", &LuaResponse::body)
                .addFunction("setBody", &LuaResponse::setBody)
                .addFunction("header", &LuaResponse::header)
                .addFunction("setHeader", &LuaResponse::setHeader)
                .addFunction("headers", &LuaResponse::headers)
                .addFunction("contentType", &LuaResponse::contentType)
                .addFunction("json", &LuaResponse::json)
            .endClass()

        .endNamespace();
}