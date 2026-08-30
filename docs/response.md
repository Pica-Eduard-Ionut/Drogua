# Response

`Drogua.Response()` creates a custom HTTP response that can be returned from a route handler.

```lua
local response = Drogua.Response()
```

## API

| Method                            | Description                   |
| --------------------------------- | ----------------------------- |
| `response:status()`               | Get the HTTP status code      |
| `response:setStatus(code)`        | Set the HTTP status code      |
| `response:body()`                 | Get the response body         |
| `response:setBody(body)`          | Set the response body         |
| `response:header(name)`           | Get a response header         |
| `response:setHeader(name, value)` | Set a response header         |
| `response:headers()`              | Get all response headers      |
| `response:contentType()`          | Get the content type          |
| `response:setContentType(type)`   | Set the content type          |
| `response:json(table)`            | Set the response body to JSON |

---

## Basic Response

Create a response, configure it, and return it from the route.

```lua
Routes.get("/hello", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setBody("Hello from Drogua")

    return response
end)
```

---

## Headers and Content Type

Set custom headers and the content type with `setHeader()` and `setContentType()`.

```lua
Routes.get("/headers", function(req)
    local response = Drogua.Response()

    response:setStatus(202)
    response:setHeader("X-Test", "LuaResponse")
    response:setHeader("X-Custom", "hello")
    response:setContentType("text/plain")
    response:setBody("Header test")

    return response
end)
```

The resulting HTTP response contains the configured status, headers, content type, and body.

Headers can also be read:

```lua
local value = response:header("X-Test")
local headers = response:headers()
```

The content type can be read with:

```lua
local type = response:contentType()
```

---

## JSON Response

`response:json()` converts a Lua table to JSON and automatically sets the content type to `application/json`.

```lua
Routes.get("/user", function(req)
    local response = Drogua.Response()
    response:setStatus(201)

    response:json({
        message = "hello",
        success = true,
        number = 123
    })

    return response
end)
```

Nested tables are supported:

```lua
response:json({
    message = "hello",
    user = {
        id = 42,
        name = "Marian",
        active = true
    }
})
```

Sequential Lua tables are converted to JSON arrays:

```lua
response:json({
    items = {
        "one",
        "two",
        "three"
    }
})
```

---

## Reading Response Values

Response values can be read after configuring the response:

```lua
local response = Drogua.Response()

response:setStatus(201)
response:setHeader("X-Test", "hello")
response:setContentType("text/plain")
response:setBody("Hello")

print(response:status())
print(response:body())
print(response:header("X-Test"))
print(response:contentType())

local headers = response:headers()
```

---

## Complete Example

```lua
local Routes = Drogua.Routes

Routes.get("/response", function(req)
    local response = Drogua.Response()

    response:setStatus(201)
    response:setHeader("X-Test", "Drogua")
    response:setContentType("text/plain")
    response:setBody("Created")

    return response
end)

Routes.get("/response/json", function(req)
    local response = Drogua.Response()

    response:setStatus(200)
    response:setHeader("X-Test", "JSON")

    response:json({
        message = "Hello from Drogua",
        success = true,
        user = {
            id = 42,
            name = "Marian"
        }
    })

    return response
end)
```

---

## Next: [Database](database.md)
