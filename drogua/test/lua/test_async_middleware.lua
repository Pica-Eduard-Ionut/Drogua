local Routes = Drogua.Routes
local Middleware = Drogua.Middleware


local function queryMiddleware(fn)
    return Middleware.create(function(req, res, next)
        fn(req, res)

        local db = Drogua.Database.get("lua_test_db")

        db:queryAsync("SELECT 1")

        next()
    end)
end


-- Middleware executes
local executeMiddleware = queryMiddleware(function(_, res)
    res:setHeader("x-middleware-executed", "true")
end)

Routes.getAsync("/lua/async-middleware/execute", function()
    return { middleware = "executed" }
end, { executeMiddleware })


-- Middleware receives request
local requestMiddleware = queryMiddleware(function(req, res)
    res:setHeader("x-middleware-path", req:path())
    res:setHeader("x-middleware-method", req:method())
end)

Routes.getAsync("/lua/async-middleware/request", function(req)
    return {
        method = req:method(),
        path = req:path()
    }
end, { requestMiddleware })


-- Middleware order
local order = {}

local middleware1 = queryMiddleware(function()
    table.insert(order, "middleware1")
end)

local middleware2 = queryMiddleware(function()
    table.insert(order, "middleware2")
end)

Routes.getAsync("/lua/async-middleware/order", function()
    table.insert(order, "handler")

    return {
        order = table.concat(order, ",")
    }
end, { middleware1, middleware2 })


-- Async middleware + async handler
local databaseMiddleware = Middleware.create(function(_, res, next)
    local db = Drogua.Database.get("lua_test_db")

    local result = db:queryAsync(
        "SELECT id, name FROM users WHERE id = ?",
        { 1 }
    )

    local rows = result:toTable()

    assert(#rows > 0)

    res:setHeader("x-middleware-user", rows[1].name)

    next()
end)

Routes.getAsync("/lua/async-middleware/db", function()
    local result = Drogua.Database.get("lua_test_db"):queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return {
        success = true,
        users = result:toTable()
    }
end, { databaseMiddleware })


-- Async middleware error
local errorMiddleware = Middleware.create(function()
    Drogua.Database.get("lua_test_db"):queryAsync(
        "SELECT * FROM definitely_missing_table"
    )
end)

Routes.getAsync("/lua/async-middleware/error", function()
    error("Handler should not run")
end, { errorMiddleware })


-- Async middleware short-circuit
local unauthorized = Middleware.create(function(_, res)
    res:setStatus(401)
    res:json({ error = "Unauthorized" })
end)

Routes.getAsync("/lua/async-middleware/short-circuit", function()
    error("Handler should not run")
end, { unauthorized })


-- Async middleware + custom response
local responseMiddleware = queryMiddleware(function() end)

Routes.getAsync("/lua/async-middleware/response", function()
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("x-test", "async")
    response:json({ success = true })

    return response
end, { responseMiddleware })


-- Sync + async middleware
local syncMiddleware = Middleware.create(function(_, res, next)
    res:setHeader("x-sync-middleware", "true")
    next()
end)

local asyncMiddleware = queryMiddleware(function(_, res)
    res:setHeader("x-async-middleware", "true")
end)

Routes.getAsync("/lua/async-middleware/mixed", function()
    return { success = true }
end, { syncMiddleware, asyncMiddleware })
