local Routes = Drogua.Routes

Routes.get("/http-test/target/get", function(req)
    return {
        success = true,
        method = req:method(),
        message = "GET request received",
        user_agent = req:header("User-Agent") or "missing"
    }
end)

Routes.post("/http-test/target/post", function(req)
    return {
        success = true,
        method = req:method(),
        message = "JSON POST received",
        received = req:json()
    }
end)

Routes.post("/http-test/target/raw", function(req)
    return {
        success = true,
        method = req:method(),
        message = "Raw POST received",
        raw_body = req:body(),
        content_type = req:header("Content-Type") or "missing"
    }
end)

Routes.get("/http-test/target/error", function(req)
    local resp = Drogua.Response()

    resp:setStatus(500)
    resp:setContentType("application/json")
    resp:setBody(
        '{"success":false,"error":"intentional test error"}'
    )

    return resp
end)

Routes.get("/http-test/target/headers", function(req)
    local resp = Drogua.Response()

    resp:setStatus(200)
    resp:setHeader("X-Drogua-Test", "hello")
    resp:setHeader("X-Drogua-Number", "123")
    resp:setContentType("application/json")
    resp:setBody('{"success":true}')

    return resp
end)