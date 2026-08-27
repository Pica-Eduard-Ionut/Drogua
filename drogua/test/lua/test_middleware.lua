local Routes = Drogua.Routes
local Middleware = Drogua.Middleware

-- ============================================================
-- Middleware executes
-- ============================================================
local executeMiddleware = Middleware.create(function(req, res, next)
    res:setHeader("X-Middleware-Executed", "true")
    next()
end)

Routes.get(
    "/lua/middleware/execute",
    function(req)
        return {
            middleware = "executed"
        }
    end,
    {
        executeMiddleware
    }
)

-- ============================================================
-- Middleware receives request
-- ============================================================
local requestMiddleware = Middleware.create(function(req, res, next)
    res:setHeader("X-Middleware-Path", req:path())
    res:setHeader("X-Middleware-Method", req:method())
    next()
end)

Routes.get(
    "/lua/middleware/request",
    function(req)
        return {
            method = req:method(),
            path = req:path()
        }
    end,
    {
        requestMiddleware
    }
)

-- ============================================================
-- Middleware order
-- ============================================================
local order = {}

local middleware1 = Middleware.create(function(req, res, next)
    table.insert(order, "middleware1")
    next()
    table.insert(order, "middleware1-after")
end)

local middleware2 = Middleware.create(function(req, res, next)
    table.insert(order, "middleware2")
    next()
    table.insert(order, "middleware2-after")
end)

Routes.get(
    "/lua/middleware/order",
    function(req)
        table.insert(order, "handler")

        return {
            order = table.concat(order, ",")
        }
    end,
    {
        middleware1,
        middleware2
    }
)