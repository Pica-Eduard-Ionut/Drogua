# Routes

Routes are registered through `Drogua.Routes`.

```lua
local Routes = Drogua.Routes
```

## HTTP Methods

Drogua currently supports:

```lua
Routes.get(path, handler)
Routes.post(path, handler)
Routes.put(path, handler)
Routes.delete(path, handler)
Routes.patch(path, handler)
```

Example:

```lua
Routes.get("/hello", function(req)
    return {
        message = "Hello from Drogua"
    }
end)
```

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
end,
{ auth, logging })
```

Middleware executes in the order provided:

```text
auth → logging → handler
```

Each middleware receives:

```lua
function(req, res, next)
```

and must call `next()` to continue to the next middleware or route handler.

See [Middleware](middleware.md).

---

## Complete Example

```lua
local Routes = Drogua.Routes
local Middleware = Drogua.Middleware

local auth = Middleware.create(function(req, res, next)
    print("auth")
    next()
end)

local logging = Middleware.create(function(req, res, next)
    print("logging")
    next()
end)

-- Simple route
Routes.get("/hello", function(req)
    return {
        message = "Hello from Drogua"
    }
end)

-- Path parameter
Routes.get("/users/{id}", function(req, id)
    return {
        id = id
    }
end)

-- Multiple path parameters + middleware
Routes.get("/users/{user}/posts/{post}", function(req, user, post)
    return {
        user = user,
        post = post
    }
    end,
    {
        auth,
        logging
    }
)

-- Static JSON response
Routes.get("/config", {
    name = "Drogua",
    version = 1
})

-- Custom response
Routes.get("/created", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setBody("Created")

    return response
end)
```

---

## Next: [Request](request.md)
