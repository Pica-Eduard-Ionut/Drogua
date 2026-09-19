local Http = Drogua.Http
local Routes = Drogua.Routes

Routes.getAsync("/lua/async-httprequest/get", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/get",
        { method = "GET" }
    )

    return {
        status = r:status(),
        ok = r:ok(),
        body = r:body()
    }
end)

Routes.getAsync("/lua/async-httprequest/json", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/get",
        { method = "GET" }
    )

    local data = r:json()

    return {
        status = r:status(),
        success = data.success,
        method = data.method,
        message = data.message,
        user_agent = data.user_agent
    }
end)

Routes.getAsync("/lua/async-httprequest/post", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/post",
        {
            method = "POST",
            headers = {
                ["Content-Type"] = "application/json"
            },
            body = {
                operation = "async-test",
                values = { 10, 20, 30 },
                nested = {
                    enabled = true
                }
            }
        }
    )

    local data = r:json()

    return {
        status = r:status(),
        ok = r:ok(),
        received = data.received
    }
end)

Routes.getAsync("/lua/async-httprequest/raw", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/raw",
        {
            method = "POST",
            headers = {
                ["Content-Type"] = "text/plain"
            },
            body = "Hello from async Lua HTTP"
        }
    )

    local data = r:json()

    return {
        status = r:status(),
        ok = r:ok(),
        raw_body = data.raw_body,
        content_type = data.content_type
    }
end)

Routes.getAsync("/lua/async-httprequest/headers", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/headers",
        { method = "GET" }
    )

    local headers = r:headers()

    return {
        status = r:status(),
        ok = r:ok(),
        test_header = headers["x-drogua-test"],
        test_number = headers["x-drogua-number"]
    }
end)

Routes.getAsync("/lua/async-httprequest/error", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/error",
        { method = "GET" }
    )

    local data = r:json()

    return {
        status = r:status(),
        ok = r:ok(),
        error = data.error
    }
end)

Routes.getAsync("/lua/async-httprequest/retry", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/get",
        {
            method = "GET",
            retry = {
                count = 2,
                delay = 100
            }
        }
    )

    return {
        status = r:status(),
        ok = r:ok(),
        body = r:json()
    }
end)

Routes.getAsync("/lua/async-httprequest/custom-headers", function(req)
    local r = Http.requestAsync(
        "http://127.0.0.1:18080/http-test/target/headers",
        {
            method = "GET",
            headers = {
                ["X-Drogua-Test"] = "custom-header"
            }
        }
    )

    local headers = r:headers()

    return {
        status = r:status(),
        ok = r:ok(),
        test_header = headers["x-drogua-test"],
        test_number = headers["x-drogua-number"]
    }
end)