Drogua.print("Hello from Lua!")
Drogua.print("This message is coming from app.lua")

-- testing routes
Drogua.Routes.get("/test/get", {
    method = "GET",
    message = "GET works"
})

Drogua.Routes.post("/test/post", {
    method = "POST",
    message = "POST works"
})

Drogua.Routes.put("/test/put", {
    method = "PUT",
    message = "PUT works"
})

Drogua.Routes.delete("/test/delete", {
    method = "DELETE",
    message = "DELETE works"
})

Drogua.Routes.patch("/test/patch", {
    method = "PATCH",
    message = "PATCH works"
})

Drogua.Routes.get("/user", function() 
    return {
        message = "test"
    }
end)

local users = { [1] = "Marian", [2] = "Iulia"}

-- currently working using Query params (?name=value)
Drogua.Routes.get("/user", function(req)
    local id = tonumber(req:param("id"))
    local t = req:params()

    return {
        id = tostring(id),
        user = users[id],
        params = t
    }
end)

Drogua.Routes.get("/user/{id}", function(req, id)
    return {
        id = id,
        user = users[tonumber(id)]
    }
end)

-- testing more params
Drogua.Routes.get("/param/{id}/1/{test}/{other}", function(req, id, test, other)
    return {
        id = id,
        test = test,
        other = other
    }
end)

-- testing new methods

Drogua.Routes.get("/test/request/{id}", function(req, id)

    print("========== REQUEST TEST ==========")

    -- Request metadata
    print("method:", req:method())
    print("path:", req:path())
    print("secure:", req:secure())
    print("ip:", req:ip())

    -- Route parameter
    print("id argument:", id)
    print("id via query():", req:query("id"))
    print("all queryParams():")
    local params = req:queryParams()
    for k,v in pairs(params) do
        print(" |-> key: " .. k .. " -> " .. v)
    end

    -- Headers
    print("User-Agent:", req:header("User-Agent"))
    print("Content-Type:", req:header("Content-Type"))

    print("all headers:")
    for key, value in pairs(req:headers()) do
        print("  ", key, "=", value)
    end

    -- Cookies
    print("session cookie:", req:cookie("session"))

    print("all cookies:")
    for key, value in pairs(req:cookies()) do
        print("  ", key, "=", value)
    end

    -- Body
    print("body:")
    print(req:body())

    -- JSON
    local json = req:json()

    if json then
        print("JSON:")

        for key, value in pairs(json) do
            print("  ", key, "=", value, "type:", type(value))
        end

        if json.user then
            print("JSON user:")
            print("  name:", json.user.name)
            print("  age:", json.user.age)
            print("  active:", json.user.active)
        end
    else
        print("No JSON body")
    end

    print("==================================")

    return {
        status = 200,
        body = {
            ok = true,
            message = "LuaRequest works!",
            id = id,
            method = req:method(),
            path = req:path(),
            ip = req:ip(),
            secure = req:secure()
        }
    }
end)