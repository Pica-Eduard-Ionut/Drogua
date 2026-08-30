# Request

Every route handler receives a `req` object representing the current HTTP request.

```lua
Routes.get("/hello", function(req)
    return {
        method = req:method(),
        path = req:path()
    }
end)
```

## API

| Method              | Description                                  |
| ------------------- | -------------------------------------------- |
| `req:method()`      | HTTP method                                  |
| `req:path()`        | Request path                                 |
| `req:secure()`      | Whether the request uses a secure connection |
| `req:ip()`          | Client IP address                            |
| `req:query(name)`   | Get a query parameter                        |
| `req:queryParams()` | Get all query parameters                     |
| `req:header(name)`  | Get a request header                         |
| `req:headers()`     | Get all request headers                      |
| `req:cookie(name)`  | Get a cookie                                 |
| `req:cookies()`     | Get all cookies                              |
| `req:body()`        | Get the raw request body                     |
| `req:json()`        | Parse the request body as JSON               |

---

## Request Information

```lua
Routes.get("/request", function(req)
    return {
        method = req:method(),
        path = req:path(),
        secure = req:secure(),
        ip = req:ip()
    }
end)
```

---

## Query Parameters

Get a single parameter:

```lua
Routes.get("/search", function(req)
    return {
        name = req:query("name")
    }
end)
```

For:

```text
GET /search?name=Drogua
```

`req:query("name")` returns `"Drogua"`.

Get all parameters with `queryParams()`:

```lua
Routes.get("/search", function(req)
    local params = req:queryParams()
    return {
        name = params.name
    }
end)
```

---

## Headers

Get a single header:

```lua
Routes.get("/headers", function(req)
    return {
        value = req:header("X-Test")
    }
end)
```

Get all headers:

```lua
Routes.get("/headers", function(req)
    local headers = req:headers()

    return {
        value = headers["x-test"]
    }
end)
```

---

## Cookies

Get a single cookie:

```lua
Routes.get("/session", function(req)
    return {
        value = req:cookie("session")
    }
end)
```

Get all cookies:

```lua
Routes.get("/session", function(req)
    local cookies = req:cookies()

    return {
        value = cookies.session
    }
end)
```

---

## Request Body

`req:body()` returns the raw request body.

```lua
Routes.post("/body", function(req)
    return {
        body = req:body()
    }
end)
```

For:

```text
POST /body

hello
```

the returned value is `"hello"`.

---

## JSON

`req:json()` converts a JSON request body into Lua values.

```lua
Routes.post("/user", function(req)
    local data = req:json()

    return {
        name = data.name,
        age = data.age
    }
end)
```

For:

```json
{
    "name": "Drogua",
    "age": 42
}
```

`data` is available as a Lua table:

```lua
{
    name = "Drogua",
    age = 42
}
```

JSON objects become Lua tables and JSON arrays become Lua arrays.

---

## Complete Example

```lua
local Routes = Drogua.Routes

Routes.post("/request", function(req)
    local query = req:queryParams()
    local headers = req:headers()
    local cookies = req:cookies()

    return {
        method = req:method(),
        path = req:path(),
        secure = req:secure(),
        ip = req:ip(),

        query = query.name,
        header = headers["x-test"],
        cookie = cookies.session,

        body = req:body()
    }
end)

Routes.post("/request/json", function(req)
    local data = req:json()

    return {
        name = data.name,
        age = data.age
    }
end)
```

---

## Next: [Response](response.md)
