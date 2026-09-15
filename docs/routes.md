# Routes

Routes are registered through `Drogua.Routes`.

```lua
local Routes = Drogua.Routes
```

## HTTP Methods

Drogua supports synchronous and asynchronous route handlers.

### Synchronous Routes

```lua
Routes.get(path, handler)
Routes.post(path, handler)
Routes.put(path, handler)
Routes.delete(path, handler)
Routes.patch(path, handler)
```

### Asynchronous Routes

```lua
Routes.getAsync(path, handler)
Routes.postAsync(path, handler)
Routes.putAsync(path, handler)
Routes.deleteAsync(path, handler)
Routes.patchAsync(path, handler)
```

Use the `Async` variants when the route needs to perform asynchronous operations, such as asynchronous database queries or asynchronous transactions.

Example:

```lua
Routes.get("/hello", function(req)
    return {
        message = "Hello from Drogua"
    }
end)
```

An asynchronous route uses the same handler style:

```lua
Routes.getAsync("/async", function(req)
    return {
        message = "async works"
    }
end)
```

The main difference is that an async route can execute Drogua's asynchronous APIs such as `queryAsync()` and `beginAsync()`.

---

## Route Handlers

A handler receives the request as its first argument.

```lua
Routes.get("/users", function(req)
    return {
        method = req:method(),
        path = req:path()
    }
end)
```

Async handlers use the same request API:

```lua
Routes.getAsync("/users", function(req)
    return {
        method = req:method(),
        path = req:path()
    }
end)
```

See [Request](request.md) for the request API.

---

## Returning a Lua Table

Returning a Lua table automatically produces a JSON response.

```lua
Routes.get("/user", function(req)
    return {
        id = 42,
        name = "Marian",
        active = true
    }
end)
```

Nested tables and arrays are supported:

```lua
Routes.get("/data", function(req)
    return {
        user = {
            id = 42,
            name = "Marian"
        },

        items = {
            "one",
            "two",
            "three"
        }
    }
end)
```

This works the same way for asynchronous routes:

```lua
Routes.getAsync("/async/data", function(req)
    return {
        success = true,
        result = "async works"
    }
end)
```

---

## Static Table Routes

A route can also receive a table directly instead of a handler:

```lua
Routes.get("/config", {
    name = "Drogua",
    version = 1,
    active = true
})
```

This creates a JSON response without executing a Lua handler.

Static table routes are synchronous route definitions. Use a handler with `getAsync()` or another `Async` route method when asynchronous work is required.

---

## Path Parameters

Named path parameters use `{name}`.

```lua
Routes.get("/users/{id}", function(req, id)
    return {
        id = id
    }
end)
```

A request to:

```text
GET /users/42
```

passes `"42"` to the `id` argument:

```lua
function(req, id)
```

Path parameters are always passed **after `req`**, in the same order they appear in the route.

This works identically with asynchronous routes:

```lua
Routes.getAsync("/users/{id}", function(req, id)
    return {
        id = id
    }
end)
```

### Multiple Parameters

```lua
Routes.get("/users/{user}/posts/{post}", function(req, user, post)
    return {
        user = user,
        post = post
    }
end)
```

For:

```text
GET /users/marian/posts/42
```

the handler receives:

```lua
req
user = "marian"
post = "42"
```

The same parameter behavior applies to async routes:

```lua
Routes.getAsync("/users/{user}/posts/{post}", function(req, user, post)
    return {
        user = user,
        post = post
    }
end)
```

### Multiple Parameters With Different Names

```lua
Routes.get("/orders/{orderId}/items/{itemId}", function(req, orderId, itemId)
    return {
        order = orderId,
        item = itemId
    }
end)
```

For:

```text
GET /orders/100/items/25
```

the values are:

```lua
orderId = "100"
itemId = "25"
```

Drogua currently supports up to **6 path parameters** per route.

---

## Asynchronous Routes

Asynchronous routes are registered using the `Async` variant of the HTTP method:

```lua
Routes.getAsync(path, handler)
```

For example:

```lua
Routes.getAsync("/async", function(req)
    return {
        message = "async works"
    }
end)
```

An async route can execute asynchronous Drogua APIs during the request.

For example, an asynchronous database query can be performed directly inside the handler:

```lua
Routes.getAsync("/async-db", function(req)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return {
        message = "database completed",
        result = result:toTable()
    }
end)
```

The asynchronous database operation completes before the handler continues to construct its response.

See [Database](database.md) for the database API and asynchronous queries.

---

## Async Database Queries

Async routes can use `queryAsync()` for database operations.

```lua
Routes.getAsync("/async-db-params", function(req)

    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT * FROM users WHERE id = ?",
        { 1 }
    )

    return {
        success = true,
        rows = result:toTable()
    }

end)
```

Path parameters can also be used with asynchronous queries:

```lua
Routes.getAsync("/async/users/{id}", function(req, id)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT * FROM users WHERE id = ?",
        { tonumber(id) }
    )

    return {
        success = true,
        user = result:toTable()
    }
end)
```

Multiple asynchronous queries can also be executed within the same route:

```lua
Routes.getAsync("/async-multiple", function(req)
    local db = Drogua.Database.get("default")

    local users = db:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    local count = db:queryAsync([[
        SELECT COUNT(*) AS total
        FROM users
    ]])

    return {
        success = true,
        users = users:toTable(),
        count = count:toTable()
    }
end)
```

---

## Async Transactions

Asynchronous routes can create database transactions with `beginAsync()`.

```lua
Routes.getAsync("/async-begin", function(req)
    local db = Drogua.Database.get("default")

    local tx = db:beginAsync()

    local result = tx:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    tx:commit()

    return {
        success = true,
        transactionValid = tx:valid(),
        rows = result:toTable()
    }
end)
```

Transactions support asynchronous queries followed by either `commit()` or `rollback()`.

### Commit

```lua
Routes.getAsync("/async-commit", function(req)
    local db = Drogua.Database.get("default")

    local tx = db:beginAsync()

    local result = tx:queryAsync([[
        INSERT INTO users (name)
        VALUES ('Async Commit User')
    ]])

    tx:commit()

    return {
        success = true,
        transactionValid = tx:valid(),
        affected = result:affectedRows()
    }
end)
```

### Rollback

```lua
Routes.getAsync("/async-rollback", function(req)
    local db = Drogua.Database.get("default")

    local tx = db:beginAsync()

    local result = tx:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    tx:rollback()

    return {
        success = true,
        transactionValid = tx:valid(),
        rows = result:toTable()
    }
end)
```

See [Database](database.md) for the complete transaction API.

---

## Returning a `Drogua.Response`

For custom status codes, headers, content types, or bodies, return a `Drogua.Response`:

```lua
Routes.get("/created", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("X-Test", "Drogua")
    response:setContentType("text/plain")
    response:setBody("Created")

    return response
end)
```

`Drogua.Response` can also be returned from an asynchronous route:

```lua
Routes.getAsync("/async-created", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("X-Test", "async")
    response:json({
        success = true
    })

    return response
end)
```

See [Response](response.md) for the response API.

---

## Middleware

Routes can receive a list of middleware as the third argument.

```lua
local auth = Drogua.Middleware.create(function(req, res, next)
    print("auth")
    next()
end)

local logging = Drogua.Middleware.create(function(req, res, next)
    print("logging")
    next()
end)

Routes.get("/users/{id}", function(req, id)
    return {
        id = id
    }

end, { auth, logging })
```

Middleware executes in the order provided:

```text
auth -> logging -> handler
```

Each middleware receives:

```lua
function(req, res, next)
```

and must call `next()` to continue to the next middleware or route handler.

See [Middleware](middleware.md).

---

## Async Middleware

Asynchronous routes support middleware that performs asynchronous work.

For example:

```lua
local auth = Drogua.Middleware.create(function(req, res, next)
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

The middleware can perform an asynchronous database operation before calling `next()`.

Another middleware can perform its own asynchronous operation:

```lua
local logging = Drogua.Middleware.create(function(req, res, next)
    print("logging before")
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT COUNT(*) AS total FROM users"
    )

    print("logging user count:", result:toTable()[1].total)
    next()
    print("logging after")
end)
```

The middleware can then be attached to an async route:

```lua
Routes.getAsync("/async-middleware-db", function(req, res)
    print("handler")
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT id, name FROM users ORDER BY id"
    )

    return {
        success = true,
        users = result:toTable()
    }
end, {
    auth,
    logging
})
```

The execution flow is:

```text
auth before
    V
auth async database query
    V
logging before
    V
logging async database query
    V
handler
    V
logging after
    V
auth after
```

Async middleware uses the same `next()` model as regular middleware.

---

## Middleware Short-Circuiting

Middleware can stop request processing by sending a response without calling `next()`.

```lua
local auth = Drogua.Middleware.create(function(req, res, next)
    res:setStatus(401)

    res:json({
        error = "Unauthorized"
    })

    return
end)
```

A route using this middleware:

```lua
Routes.getAsync("/async-middleware-short-circuit", function(req, res)
    print("handler SHOULD NOT RUN")

    return {
        success = true
    }
end, {
    auth
})
```

will not execute the route handler because the middleware does not call `next()`.

Middleware after the short-circuiting middleware is also not executed.

The execution flow is:

```text
auth
  ↓
401 response
  ↓
request stops
```

---

## Async Middleware Errors

Errors from asynchronous operations inside middleware stop normal middleware and route execution.

For example:

```lua
local auth = Drogua.Middleware.create(function(req, res, next)
    print("auth error before")

    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT * FROM definitely_missing_table"
    )

    print("THIS SHOULD NOT RUN")
    next()
    print("THIS SHOULD NOT RUN EITHER")
end)
```

If `queryAsync()` fails, execution does not continue to `next()` or to the route handler.

This allows asynchronous database failures to propagate through Drogua's request/error handling instead of requiring every middleware to manually check for failure before continuing.

---

## Async Middleware and Response Objects

Middleware receives the same `req` and `res` objects as synchronous middleware:

```lua
function(req, res, next)
```

For example:

```lua
local auth = Drogua.Middleware.create(function(req, res, next)
    print("auth before")
    next()
    print("auth after")
end)

Routes.getAsync("/async-middleware-response", function(req, res)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("X-Test", "async")

    response:json({
        success = true
    })

    return response
end, {
    auth
})
```

An async route can therefore use either:

* a returned Lua table
* a returned `Drogua.Response`
* middleware-generated responses

---

## Route Method Reference

| HTTP Method | Synchronous       | Asynchronous           |
| ----------- | ----------------- | ---------------------- |
| GET         | `Routes.get()`    | `Routes.getAsync()`    |
| POST        | `Routes.post()`   | `Routes.postAsync()`   |
| PUT         | `Routes.put()`    | `Routes.putAsync()`    |
| DELETE      | `Routes.delete()` | `Routes.deleteAsync()` |
| PATCH       | `Routes.patch()`  | `Routes.patchAsync()`  |

Use the asynchronous variant when the route or its middleware needs Drogua's asynchronous APIs.

---

## Complete Example

```lua
local Routes = Drogua.Routes
local Middleware = Drogua.Middleware

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

-- Simple synchronous route
Routes.get("/hello", function(req)
    return {
        message = "Hello from Drogua"
    }
end)

-- Simple asynchronous route
Routes.getAsync("/async", function(req)
    return {
        message = "async works"
    }
end)

-- Async database query
Routes.getAsync("/async-db", function(req)
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
end)

-- Async route with path parameter
Routes.getAsync("/async/users/{id}", function(req, id)
    local db = Drogua.Database.get("default")
    local result = db:queryAsync(
        "SELECT * FROM users WHERE id = ?",
        { tonumber(id) }
    )

    return {
        success = true,
        user = result:toTable()
    }
end)

-- Async transaction
Routes.getAsync("/async-commit", function(req)
    local db = Drogua.Database.get("default")

    local tx = db:beginAsync()
    local result = tx:queryAsync([[
        INSERT INTO users (name)
        VALUES ('Async Commit User')
    ]])

    tx:commit()

    return {
        success = true,
        affected = result:affectedRows()
    }
end)

-- Async middleware
Routes.getAsync("/async-middleware-db", function(req)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync(
        "SELECT id, name FROM users ORDER BY id"
    )

    return {
        success = true,
        users = result:toTable()
    }
end, {
    auth,
    logging
})

-- Multiple path parameters
Routes.getAsync("/async/{a}/{b}/{c}", function(req, a, b, c)
    return {
        a = a,
        b = b,
        c = c
    }
end)

-- Custom response
Routes.getAsync("/async-created", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("X-Test", "async")

    response:json({
        success = true
    })

    return response
end)
```

---

## Summary

Drogua provides both synchronous and asynchronous route APIs.

Use synchronous routes when no asynchronous Drogua operation is required:

```lua
Routes.get("/hello", handler)
```

Use asynchronous routes when the request needs asynchronous database or transaction operations:

```lua
Routes.getAsync("/users", handler)
```

Async routes support:

* asynchronous route handlers
* asynchronous database queries with `queryAsync()`
* asynchronous transactions with `beginAsync()`
* asynchronous transaction queries with `tx:queryAsync()`
* `commit()` and `rollback()`
* middleware execution
* middleware performing asynchronous database operations
* middleware short-circuiting
* normal Lua table responses
* `Drogua.Response` responses
* path parameters

The async API keeps the same route and middleware programming model while allowing request processing to perform asynchronous I/O.

---

## Next: [Request](request.md)
