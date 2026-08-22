#include "LuaBindings.h"

#include <LuaBridge/LuaBridge.h>

#include <lua_bindings/LuaRoutes.h>
#include <lua_bindings/LuaRequest.h>
#include <lua_bindings/LuaResponse.h>
#include <lua_bindings/LuaHttpAppFramework.h>
#include <lua_bindings/LuaDatabase.h>
#include <lua_bindings/LuaResult.h>
#include <lua_bindings/LuaRow.h>

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
                .addFunction("setContentType", &LuaResponse::setContentType)
                .addFunction("json", &LuaResponse::json)
            .endClass()

            // Database functions
            .beginNamespace("Database")
                .addFunction("get", &LuaDatabase::get)
            .endNamespace()

            .beginClass<LuaDatabase>("DatabaseClient")
                .addFunction("name", &LuaDatabase::name)
                .addFunction("valid", &LuaDatabase::valid)
                .addFunction("exec", &LuaDatabase::execute)
                .addFunction("query", &LuaDatabase::queryLua)
                .addFunction("executeAffected", &LuaDatabase::executeAffected)
                .addFunction("lastInsertId", &LuaDatabase::lastInsertId)
            .endClass()

            .beginClass<LuaResult>("DatabaseResult")
                .addFunction("size", &LuaResult::size)
                .addFunction("count", &LuaResult::count)
                .addFunction("columns", &LuaResult::columns)
                .addFunction("columnName", &LuaResult::columnName)
                .addFunction("row", &LuaResult::row)
                .addFunction("affectedRows", &LuaResult::affectedRows)
                .addFunction("insertId", &LuaResult::insertId)
                .addFunction("toTable", &LuaResult::luaToTable)
                .addFunction("toString", &LuaResult::toString)
            .endClass()

            .beginClass<LuaRow>("DatabaseRow")
                .addFunction("size", &LuaRow::size)
                .addFunction("columnName", &LuaRow::columnName)
                .addFunction("isNull", static_cast<bool (LuaRow::*)(const std::string&) const>(&LuaRow::isNull))
                .addFunction("get", static_cast<std::string (LuaRow::*)(const std::string&) const>(&LuaRow::getString))
                .addFunction("toString", &LuaRow::toString)
            .endClass()
            
        .endNamespace();
}