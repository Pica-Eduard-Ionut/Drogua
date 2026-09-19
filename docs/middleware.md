# Middleware

Drogua provides Lua middleware for running code before and after a route handler.

Middleware is created from a Lua function using `Drogua.Middleware.create()` and attached to an individual route when the route is registered.

A middleware function receives three arguments:

```lua
function(req, res, next)
```

* `req` — the current `Drogua.Request`
* `res` — the current `Drogua.Response`
* `next` — a function that continues execution to the next middleware or the route handler

Middleware works with both synchronous and asynchronous routes.

## Creating Middleware

```lua
local Middleware = Drogua.Middleware

local authMiddleware = Middleware.create(function(req, res, next)
    -- middleware logic
    next()
end)
```

`Middleware.create()` expects exactly one argument, and that argument must be a Lua function.

The returned value is a `Drogua.Middleware` object that can be attached to routes.

## Attaching Middleware to a Route

Middleware is passed as the third argument to a route:

```lua
local Routes = Drogua.Routes
local Middleware = Drogua.Middleware

local middleware = Middleware.create(function(req, res, next)
    res:setHeader("X-Middleware", "true")
    next()
end)

Routes.get("/example", function(req)
    return {
        message = "Hello"
    }
end, { middleware })
```

The middleware runs before the route handler.

In this example, the response will contain:

```http
X-Middleware: true
```

The same middleware can be attached to an asynchronous route:

```lua
Routes.getAsync("/example", function(req)
    return {
        message = "Hello"
    }
end, { middleware })
```

## Request and Response Access

Middleware has access to both the request and response objects.

```lua
local middleware = Middleware.create(function(req, res, next)
    local path = req:path()
    local method = req:method()

    res:setHeader("X-Request-Path", path)
    res:setHeader("X-Request-Method", method)

    next()
end)
```

The request object provides the same request API available to route handlers, while the response object can be modified before the route handler executes.

## Calling `next()`

`next()` continues execution through the middleware chain.

```lua
local middleware = Middleware.create(function(req, res, next)
    -- Before the route handler
    next()
    -- After the route handler
end)
```

This allows middleware to perform work both **before and after** the next middleware or route handler.

For example:

```lua
local middleware = Middleware.create(function(req, res, next)
    table.insert(log, "before")
    next()
    table.insert(log, "after")
end)
```

If this is the only middleware on a route, the execution order is:

```text
middleware before
route handler
middleware after
```

If `next()` is not called, execution does not continue to the next middleware or route handler.

## Multiple Middleware

Multiple middleware can be attached to the same route by placing them in a Lua table:

```lua
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

Routes.get("/example", function(req)
    table.insert(order, "handler")

    return {
        order = table.concat(order, ",")
    }
end, { middleware1, middleware2 })
```

Middleware is executed in the order in which it appears in the table.

The execution flow is:

```text
middleware1
    V
middleware2
    V
route handler
    V
middleware2-after
    V
middleware1-after
```

Therefore, the resulting order is:

```text
middleware1, middleware2, handler, middleware2-after, middleware1-after
```

This follows the usual nested middleware model: each middleware must call `next()` to enter the next layer, and execution resumes after `next()` returns.

## Async Middleware

Middleware can also perform asynchronous operations when attached to an async route.

For example, middleware can execute an asynchronous database query before calling `next()`:

```lua
local auth = Middleware.create(function(req, res, next)
    print("auth before")

    local db = Drogua.Database.get("default")
    local result = db:queryAsync(
        "SELECT id, name FROM users WHERE id = ?",
        { 1 }
    )

    print("auth user:", result:toTable()[1].name)
    next()
    print("auth after")
end)
```

It can then be attached to an async route:

```lua
Routes.getAsync("/async-middleware-db", function(req, res)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return {
        success = true,
        users = result:toTable()
    }
end, { auth })
```

Multiple middleware can perform asynchronous operations:

```lua
local logging = Middleware.create(function(req, res, next)
    print("logging before")

    local db = Drogua.Database.get("default")
    local result = db:queryAsync(
        "SELECT COUNT(*) AS total FROM users"
    )

    print("user count:", result:toTable()[1].total)

    next()

    print("logging after")
end)
```

The execution order remains the same:

```text
auth
    V
logging
    V
route handler
```

Async operations complete before the middleware continues to `next()`.

See [Database](database.md) for the asynchronous database API.

## Middleware Short-Circuiting

Middleware can stop request processing by sending a response without calling `next()`.

```lua
local auth = Middleware.create(function(req, res, next)
    res:setStatus(401)
    res:json({
        error = "Unauthorized"
    })

    return
end)
```

For example:

```lua
Routes.getAsync("/protected", function(req, res)
    return {
        success = true
    }
end, { auth })
```

Because `auth` does not call `next()`, the route handler is not executed.

The same behavior applies to synchronous routes.

## Middleware Errors

Errors raised by a middleware function are propagated through Drogua.

```lua
local middleware = Middleware.create(function(req, res, next)
    error("Something went wrong")
    next()
end)
```

The middleware execution fails and the error is propagated back through Drogua.

This also applies to errors from asynchronous operations:

```lua
local middleware = Middleware.create(function(req, res, next)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT * FROM definitely_missing_table"
    )

    next()
end)
```

If the asynchronous database operation fails, execution does not continue to `next()` or the route handler.

## Middleware API

### `Drogua.Middleware.create(function)`

Creates a middleware from a Lua function.

```lua
local middleware = Drogua.Middleware.create(function(req, res, next)
    next()
end)
```

**Arguments**

| Argument   | Type       | Description                                            |
| ---------- | ---------- | ------------------------------------------------------ |
| `function` | `function` | Middleware function receiving `req`, `res`, and `next` |

**Returns**

A `Drogua.Middleware` object.

### Middleware function

```lua
function(req, res, next)
```

| Argument | Type              | Description                    |
| -------- | ----------------- | ------------------------------ |
| `req`    | `Drogua.Request`  | Current HTTP request           |
| `res`    | `Drogua.Response` | Current HTTP response          |
| `next`   | `function`        | Continues the middleware chain |

`next()` takes no arguments and returns no value.

## Route Middleware Syntax

Middleware is supplied as the third argument to the route registration functions:

```lua
Routes.get(path, handler, middleware)
Routes.post(path, handler, middleware)
Routes.put(path, handler, middleware)
Routes.delete(path, handler, middleware)
Routes.patch(path, handler, middleware)

Routes.getAsync(path, handler, middleware)
Routes.postAsync(path, handler, middleware)
Routes.putAsync(path, handler, middleware)
Routes.deleteAsync(path, handler, middleware)
Routes.patchAsync(path, handler, middleware)
```

The middleware argument is a Lua array containing middleware objects:

```lua
{
    middleware1,
    middleware2,
    middleware3
}
```

The order of the array determines middleware execution order.

## Example

A complete example using synchronous and asynchronous middleware:

```lua
local Routes = Drogua.Routes
local Middleware = Drogua.Middleware

local logging = Middleware.create(function(req, res, next)
    Drogua.print(req:method() .. " " .. req:path())
    next()
end)

local headers = Middleware.create(function(req, res, next)
    res:setHeader("X-Powered-By", "Drogua")
    next()
end)

Routes.get("/api/example", function(req)
    return {
        message = "Hello from the route"
    }
end, { logging, headers })

Routes.getAsync("/api/async", function(req)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT COUNT(*) AS total FROM users"
    )

    return {
        message = "Hello from the async route",
        users = result:toTable()[1].total
    }
end, { logging, headers })
```

The request flows through the middleware chain before reaching the route handler:

```text
Request
   │
   V
logging
   │
   V
headers
   │
   V
route handler
   │
   V
Response
```

If middleware performs work after `next()`, execution then unwinds in reverse order.

Async middleware follows the same middleware chain and execution model while allowing asynchronous operations such as database queries.

---
**Next:** [**Http Request**](http-request.md)