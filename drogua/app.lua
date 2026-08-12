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