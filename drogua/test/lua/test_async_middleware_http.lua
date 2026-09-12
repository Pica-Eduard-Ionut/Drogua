local function curl(path, method, raw)
    local flags = raw and "-sS -i" or "-sS -f"

    local cmd = string.format(
        "curl %s -X %s http://127.0.0.1:18080%s",
        flags,
        method,
        path
    )

    local handle = assert(io.popen(cmd), "Failed to start curl")

    local result = handle:read("*a")
    local ok, _, code = handle:close()

    if not raw then
        assert(ok, string.format(
            "curl failed: %s %s (exit %s)",
            method,
            path,
            tostring(code)
        ))
    end

    return result
end


local function assertHas(value, expected, msg)
    assert(
        value:find(expected, 1, true),
        (msg or "Missing") .. ": " .. value
    )
end


-- Middleware executes and continues after async query
local r = curl("/lua/async-middleware/execute", "GET")

assert(r == '{"middleware":"executed"}', r)


-- Middleware receives request
r = curl("/lua/async-middleware/request", "GET")

assert(
    r == '{"method":"GET","path":"/lua/async-middleware/request"}',
    r
)


-- Middleware order
r = curl("/lua/async-middleware/order", "GET")

assert(r == '{"order":"middleware1,middleware2,handler"}', r)


-- Async middleware + async handler
r = curl("/lua/async-middleware/db", "GET")

assertHas(r, '"success":true')
assertHas(r, '"users":')


-- Short circuit
r = curl("/lua/async-middleware/short-circuit", "GET", true)

assertHas(r, "401", "Expected HTTP 401")

local errorValue = string.gsub(
    r,
    '.*"error"%s*:%s*"([^"]+)".*',
    "%1"
)

assert(errorValue == "Unauthorized", errorValue)


-- Custom response
r = curl("/lua/async-middleware/response", "GET", true)

assertHas(r, "201", "Expected HTTP 201")
assertHas(r, "x-test: async")

local successValue = string.match(
    r,
    '"success"%s*:%s*(%a+)'
)

assert(successValue == "true", successValue)


-- Sync + async middleware
r = curl("/lua/async-middleware/mixed", "GET")

local mixedSuccess = string.match(
    r,
    '"success"%s*:%s*(%a+)'
)

assert(mixedSuccess == "true", mixedSuccess)
