local Routes = Drogua.Routes

Routes.get("/lua/request/method", function(req)
    return {
        method = req:method()
    }
end)

Routes.get("/lua/request/path", function(req)
    return {
        path = req:path()
    }
end)

Routes.get("/lua/request/query", function(req)
    return {
        name = req:query("name")
    }
end)

Routes.get("/lua/request/header", function(req)
    return {
        value = req:header("X-Test")
    }
end)

Routes.get("/lua/request/cookie", function(req)
    return {
        value = req:cookie("session")
    }
end)

Routes.post("/lua/request/body", function(req)
    return {
        body = req:body()
    }
end)

Routes.post("/lua/request/json", function(req)
    local data = req:json()

    return {
        name = data.name,
        age = data.age
    }
end)

Routes.get("/lua/request/all", function(req)
    local params = req:queryParams()
    local headers = req:headers()
    local cookies = req:cookies()

    return {
        method = req:method(),
        path = req:path(),
        query = params.name,
        header = headers["x-test"],
        cookie = cookies.session
    }
end)