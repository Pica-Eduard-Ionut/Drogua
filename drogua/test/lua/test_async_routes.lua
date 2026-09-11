local Routes = Drogua.Routes

Routes.getAsync("/lua/async/get", function(req)
    return { method = req:method() }
end)

Routes.postAsync("/lua/async/post", function(req)
    return { method = req:method() }
end)

Routes.putAsync("/lua/async/put", function(req)
    return { method = req:method() }
end)

Routes.deleteAsync("/lua/async/delete", function(req)
    return { method = req:method() }
end)

Routes.patchAsync("/lua/async/patch", function(req)
    return { method = req:method() }
end)

Routes.getAsync("/lua/async/request", function(req)
    return { method = req:method(), path = req:path() }
end)
