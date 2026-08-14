local Routes = Drogua.Routes

Routes.get("/lua/get", function(req)
    return {
        method = req:method()
    }
end)

Routes.post("/lua/post", function(req)
    return {
        method = req:method()
    }
end)

Routes.put("/lua/put", function(req)
    return {
        method = req:method()
    }
end)

Routes.delete("/lua/delete", function(req)
    return {
        method = req:method()
    }
end)

Routes.patch("/lua/patch", function(req)
    return {
        method = req:method()
    }
end)

Routes.get("/lua/table", {
    name = "Drogua",
    value = 42,
    active = true
})

Routes.get("/lua/users/{id}", function(req, id)
    return {
        id = id
    }
end)

Routes.get("/lua/users/{user}/posts/{post}", function(req, user, post)
    return {
        user = user,
        post = post
    }
end)

Routes.get("/lua/request", function(req)
    return {
        method = req:method(),
        path = req:path()
    }
end)

Routes.get("/lua/six/{p1}/{p2}/{p3}/{p4}/{p5}/{p6}", function(req, p1, p2, p3, p4, p5, p6)
    return {
        p1 = p1,
        p2 = p2,
        p3 = p3,
        p4 = p4,
        p5 = p5,
        p6 = p6
    }
end)