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
