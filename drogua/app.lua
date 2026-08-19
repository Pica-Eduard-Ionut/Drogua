Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", 5555)
    :setThreadNum(2)
    --:enableRunAsDaemon()

Drogua.print("Hello from Lua!")
Drogua.print("This message is coming from app.lua")

Drogua.Routes.get("/test/table", function(req)
    return {
        message = "hello from lua",
        status = "ok",
        number = 42,
        nested = {
            foo = "bar"
        }
    }
end)

Drogua.Routes.get("/test/response", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setBody("Hello from LuaResponse!")
    response:setHeader("X-Test", "LuaResponse")
    response:setContentType("text/plain")

    return response
end)

Drogua.Routes.get("/test/json-response", function(req)
    local response = Drogua.Response()

    response:setStatus(202)
    response:setHeader("X-Test", "JSON")
    response:json({
        message = "hello",
        success = true,
        number = 123,
        nested = {
            value = "works"
        }
    })

    return response
end)

Drogua.Routes.get("/test/headers", function(req)
    local response = Drogua.Response()

    response:setStatus(200)
    response:setHeader("X-Custom-Header", "hello")
    response:setHeader("X-Another-Header", "world")
    response:setContentType("text/plain")
    response:setBody("Check the response headers!")

    return response
end)

Drogua.app():run()