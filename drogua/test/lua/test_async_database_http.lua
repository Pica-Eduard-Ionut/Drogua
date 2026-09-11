local function curl(path, method)
    local command = "curl -sS -f -X " .. method .. " http://127.0.0.1:18080" .. path
    local handle = assert(io.popen(command), "Failed to start curl")
    local result = handle:read("*a")
    local success, _, exitCode = handle:close()

    assert(success, "curl failed for " .. method .. " " .. path .. " (exit code: " .. tostring(exitCode) .. ")")
    return result
end

-- ---------------------------------------------------------
-- Async query
-- ---------------------------------------------------------

assert(curl("/lua/async-database/query", "GET") == '{"columns":2,"count":2,"size":2}')

-- ---------------------------------------------------------
-- Async result -> Lua table
-- ---------------------------------------------------------

assert(curl("/lua/async-database/table", "GET") == '[{"id":"1","name":"Alice"},{"id":"2","name":"Bob"}]')

-- ---------------------------------------------------------
-- Async parameterized query
-- ---------------------------------------------------------

assert(curl("/lua/async-database/params", "GET") == '[{"id":"1","name":"Alice"}]')

-- ---------------------------------------------------------
-- Multiple async queries in one route
-- ---------------------------------------------------------

assert(curl("/lua/async-database/multiple", "GET") == '{"first":"Alice","second":"Bob"}')

-- ---------------------------------------------------------
-- Async INSERT
-- ---------------------------------------------------------

local insertResult = curl("/lua/async-database/insert", "GET")

assert(insertResult == '{"affectedRows":1,"insertId":3}', "Unexpected async insert result: " .. insertResult)

-- ---------------------------------------------------------
-- Async database error
--
-- curl -f returns failure for the HTTP error response,
-- so test the error route separately without -f.
-- ---------------------------------------------------------

local command = "curl -sS -o /tmp/drogua_async_database_error.txt -w '%{http_code}' http://127.0.0.1:18080/lua/async-database/error"
local handle = assert(io.popen(command), "Failed to start curl")
local statusCode = handle:read("*a")
local success, _, exitCode = handle:close()

assert(success, "curl failed while testing async database error (exit code: " .. tostring(exitCode) .. ")")
assert(statusCode == "500", "Expected HTTP 500 from async database error, got " .. tostring(statusCode))
