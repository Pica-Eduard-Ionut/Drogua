Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", 5555)
    :setThreadNum(2)
    --:enableRunAsDaemon()

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

Drogua.Routes.get("/user/{id}", function(req, id) 
    return {
        id = id
    }
end)

Drogua.app():run()