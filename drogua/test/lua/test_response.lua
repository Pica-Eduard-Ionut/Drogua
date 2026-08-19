local Routes = Drogua.Routes

-- Basic response
Routes.get("/lua/response/basic", function(req)
    local response = Drogua.Response()
    response:setStatus(201)
    response:setBody("Hello from LuaResponse!")

    return response
end)

-- Response with custom headers and content type
Routes.get("/lua/response/headers", function(req)
    local response = Drogua.Response()
    response:setStatus(202)
    response:setHeader("X-Test", "LuaResponse")
    response:setHeader("X-Custom", "hello")
    response:setContentType("text/plain")
    response:setBody("Header test")

    return response
end)

-- JSON response
Routes.get("/lua/response/json", function(req)
    local response = Drogua.Response()
    response:setStatus(201)
    response:setHeader("X-Test", "JSON")
    response:json({
        message = "hello",
        success = true,
        number = 123
    })

    return response
end)

-- Nested JSON response
Routes.get("/lua/response/json-nested", function(req)
    local response = Drogua.Response()
    response:setStatus(202)

    response:json({
        message = "hello",
        user = {
            id = 42,
            name = "Marian",
            active = true
        }
    })

    return response
end)

-- JSON array response
Routes.get("/lua/response/json-array", function(req)
    local response = Drogua.Response()
    response:setStatus(200)

    response:json({
        items = {
            "one",
            "two",
            "three"
        }
    })

    return response
end)
