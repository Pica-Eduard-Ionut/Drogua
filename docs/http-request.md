# HTTP Request

Drogua provides an asynchronous HTTP client through `Drogua.Http`.

HTTP requests are performed using Drogon's HTTP client and can be used from Drogua's asynchronous routes.

The main API is:

```lua
Drogua.Http.requestAsync(url, options)
```

`requestAsync()` suspends the current asynchronous route while the HTTP request is in progress and resumes the route when the response is available.

---

## Making a Request

A basic GET request:

```lua
local Http = Drogua.Http

Routes.getAsync("/example", function(req)
    local response = Http.requestAsync("http://example.com/api/users", {
            method = "GET"
        }
    )

    return {
        status = response:status(),
        body = response:body()
    }
end)
```

`requestAsync()` returns a `HttpResult` object containing the HTTP response.

The function is intended to be used from asynchronous routes created with:

```lua
Routes.getAsync()
Routes.postAsync()
Routes.putAsync()
Routes.deleteAsync()
```

It must not be used from a normal synchronous route.

---

## Request Options

The second argument to `requestAsync()` is an optional Lua table.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET",
        headers = {
            ["Accept"] = "application/json"
        }
    }
)
```

The available options are:

| Option | Type | Description |
|---|---|---|
| `method` | string | HTTP method |
| `headers` | table | Request headers |
| `body` | string/table | Request body |
| `retry` | table | Retry configuration |

---

## HTTP Methods

The `method` option specifies the HTTP method.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)
```

Supported methods include:

```text
GET
POST
PUT
DELETE
PATCH
HEAD
OPTIONS
```

The method name is case-insensitive.

For example:

```lua
method = "POST"
```

and:

```lua
method = "post"
```

both create a POST request.

If the method is not one of the explicitly supported methods, the request defaults to `GET`.

---

## Request Headers

Request headers can be specified using the `headers` table.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET",
        headers = {
            ["Accept"] = "application/json",
            ["X-Drogua-Test"] = "hello"
        }
    }
)
```

Each key becomes a request header.

For example:

```lua
headers = {
    ["Authorization"] = "Bearer token",
    ["Accept"] = "application/json"
}
```

---

## Request Body

String bodies can be sent using the `body` option.

```lua
local response = Http.requestAsync(
    "http://example.com/api/message",
    {
        method = "POST",
        body = "Hello from Drogua"
    }
)
```

The `Content-Type` can be specified using request headers:

```lua
local response = Http.requestAsync(
    "http://example.com/api/message",
    {
        method = "POST",
        headers = {
            ["Content-Type"] = "text/plain"
        },
        body = "Hello from Drogua"
    }
)
```

---

## JSON Request Bodies

A Lua table can be supplied as the request body.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "POST",
        headers = {
            ["Content-Type"] = "application/json"
        },
        body = {
            name = "Alice",
            age = 30
        }
    }
)
```

Drogua converts the Lua table to JSON before sending the request.

The resulting request body is equivalent to:

```json
{
    "name": "Alice",
    "age": 30
}
```

When a Lua table is used as the body, Drogua automatically configures the request as JSON.

---

## HTTP Response

`requestAsync()` returns a `HttpResult`.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)
```

The response object provides methods for accessing the HTTP status, body, headers, and JSON data.

### HttpResult API

| Method | Description |
|---|---|
| `status()` | Returns the HTTP status code |
| `statusMessage()` | Returns the HTTP status message |
| `body()` | Returns the response body |
| `headers()` | Returns response headers as a Lua table |
| `ok()` | Returns whether the status code is in the 2xx range |
| `json()` | Parses a JSON response into Lua values |
| `toTable()` | Converts the response to a Lua table |

---

## Status Code

Use `status()` to retrieve the HTTP status code.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)

print(response:status())
```

For example:

```text
200
```

---

## Status Message

Use `statusMessage()` to retrieve the HTTP status message.

```lua
print(response:statusMessage())
```

For a successful request this may return:

```text
OK
```

---

## Response Body

Use `body()` to retrieve the raw response body.

```lua
local response = Http.requestAsync(
    "http://example.com/api/message",
    {
        method = "GET"
    }
)

print(response:body())
```

For a JSON response, `body()` returns the JSON as a string.

For example:

```json
{"success":true,"message":"Hello"}
```

---

## Response Headers

Use `headers()` to retrieve response headers as a Lua table.

```lua
local response = Http.requestAsync(
    "http://example.com/api/message",
    {
        method = "GET"
    }
)

local headers = response:headers()

print(headers["content-type"])
```

Header names are exposed in lowercase.

For example, a response containing:

```text
X-Drogua-Test: hello
X-Drogua-Number: 123
```

can be accessed as:

```lua
local headers = response:headers()

print(headers["x-drogua-test"])
print(headers["x-drogua-number"])
```

---

## Checking for a Successful Response

Use `ok()` to check whether the HTTP status code is in the `2xx` range.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)

if response:ok() then
    print("Request succeeded")
else
    print("Request failed")
end
```

`ok()` returns `true` for status codes from `200` through `299`.

For example:

```text
200 -> true
201 -> true
204 -> true
400 -> false
404 -> false
500 -> false
```

---

## JSON Responses

Use `json()` when the response contains JSON.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)

local data = response:json()

print(data.name)
print(data.email)
```

For example, if the server returns:

```json
{
    "name": "Alice",
    "age": 30
}
```

the JSON values can be accessed directly from Lua:

```lua
print(data.name)
print(data.age)
```

Nested JSON objects become Lua tables:

```lua
local data = response:json()

print(data.user.name)
```

JSON arrays are converted to Lua tables:

```lua
local data = response:json()

for i, user in ipairs(data.users) do
    print(user.name)
end
```

`json()` returns an empty Lua reference when the response is not valid JSON or does not have the JSON content type.

---

## Converting the Response to a Table

Use `toTable()` when you want the complete HTTP response represented as a Lua table.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)

local result = response:toTable()
```

The resulting table contains:

```lua
{
    status = 200,
    statusMessage = "OK",
    body = "...",
    headers = {
        ...
    },
    ok = true
}
```

---

## POST Request

A complete POST example:

```lua
Routes.getAsync("/create-user", function(req)

    local response = Http.requestAsync(
        "http://example.com/api/users",
        {
            method = "POST",
            headers = {
                ["Content-Type"] = "application/json"
            },
            body = {
                name = "Alice",
                email = "alice@example.com"
            }
        }
    )

    return {
        status = response:status(),
        ok = response:ok(),
        body = response:body()
    }
end)
```

---

## Reading a JSON Response

A common pattern is to check the status and then read the JSON response.

```lua
Routes.getAsync("/users", function(req)
    local response = Http.requestAsync(
        "http://example.com/api/users",
        {
            method = "GET",
            headers = {
                ["Accept"] = "application/json"
            }
        }
    )

    if not response:ok() then
        return {
            status = response:status(),
            error = response:body()
        }
    end

    local data = response:json()

    return data
end)
```

---

## Retry

Requests can specify retry behavior using the `retry` option.

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET",
        retry = {
            count = 2,
            delay = 100
        }
    }
)
```

The retry options are:

| Option | Type | Description |
|---|---|---|
| `count` | number | Maximum number of retries |
| `delay` | number | Delay between retries in milliseconds |

For example:

```lua
retry = {
    count = 3,
    delay = 500
}
```

allows up to three retries with a 500 millisecond delay.

Retries apply to request failures reported by the HTTP client. A normal HTTP response such as `500 Internal Server Error` is returned as a response and can be inspected using `status()` and `ok()`.

---

## HTTP Errors

HTTP error status codes are returned as normal HTTP responses.

For example:

```lua
local response = Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)

if not response:ok() then
    print("HTTP status:", response:status())
    print("Response:", response:body())
end
```

A `500` response therefore does not automatically become a Lua exception.

You can inspect it normally:

```lua
local response = Http.requestAsync(...)

return {
    status = response:status(),
    ok = response:ok(),
    body = response:body()
}
```

Transport-level failures, such as an inability to complete the HTTP request, are reported as an error from `requestAsync()`.

---

## Complete Example

The following example sends a JSON POST request and reads the JSON response:

```lua
local Http = Drogua.Http
local Routes = Drogua.Routes

Routes.getAsync("/users/create", function(req)

    local response = Http.requestAsync(
        "http://127.0.0.1:18080/api/users",
        {
            method = "POST",

            headers = {
                ["Content-Type"] = "application/json",
                ["Accept"] = "application/json"
            },

            body = {
                name = "Alice",
                email = "alice@example.com"
            }
        }
    )

    if not response:ok() then
        return {
            success = false,
            status = response:status(),
            error = response:body()
        }
    end

    local data = response:json()

    return {
        success = true,
        status = response:status(),
        data = data
    }
end)
```

---

## Asynchronous Execution

`requestAsync()` is integrated with Drogua's asynchronous route system.

For example:

```lua
Routes.getAsync("/dashboard", function(req)

    local users = Drogua.Database
        .get("default")
        :queryAsync("SELECT * FROM users")

    local response = Drogua.Http.requestAsync(
        "http://example.com/api/status",
        {
            method = "GET"
        }
    )

    return {
        users = users:toTable(),
        external_status = response:status(),
        external_data = response:json()
    }
end)
```

The HTTP request suspends the asynchronous route while the operation is in progress and resumes the route when the response is available.

---

## URL Handling

`requestAsync()` accepts HTTP and HTTPS URLs.

Examples:

```lua
Http.requestAsync(
    "http://example.com/api/users",
    {
        method = "GET"
    }
)
```

and:

```lua
Http.requestAsync(
    "https://example.com/api/users",
    {
        method = "GET"
    }
)
```

The HTTP client is created and reused internally by Drogua.

---

## API Summary

### `Drogua.Http`

| Method | Description |
|---|---|
| `requestAsync(url, options)` | Performs an asynchronous HTTP request |

### Request Options

| Option | Type | Description |
|---|---|---|
| `method` | string | HTTP method |
| `headers` | table | Request headers |
| `body` | string/table | Request body |
| `retry` | table | Retry configuration |

### `HttpResult`

| Method | Description |
|---|---|
| `status()` | HTTP status code |
| `statusMessage()` | HTTP status message |
| `body()` | Raw response body |
| `headers()` | Response headers |
| `ok()` | Whether the response is 2xx |
| `json()` | Parse JSON response |
| `toTable()` | Convert response to a Lua table |

