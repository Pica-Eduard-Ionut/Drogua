local function curl(path, method)
    local command =
        "curl -sS -f -X " .. method ..
        " http://127.0.0.1:18080" .. path

    local handle = assert(
        io.popen(command),
        "Failed to start curl"
    )

    local result = handle:read("*a")
    local success, _, exitCode = handle:close()

    assert(
        success,
        "curl failed for " .. method .. " " .. path ..
        " (exit code: " .. tostring(exitCode)
    )
    

    return result
end

-- ============================================================
-- Middleware executes
-- ============================================================
local executeResult = curl("/lua/middleware/execute", "GET")

assert(
    executeResult == '{"middleware":"executed"}',
    "Unexpected execute response: " .. executeResult
)

-- ============================================================
-- Middleware receives request
-- ============================================================
local requestResult = curl("/lua/middleware/request", "GET")

assert(
    requestResult == '{"method":"GET","path":"/lua/middleware/request"}',
    "Unexpected request response: " .. requestResult
)

-- ============================================================
-- Middleware order
-- ============================================================
local orderResult = curl("/lua/middleware/order", "GET")

assert(
    orderResult == '{"order":"middleware1,middleware2,handler"}',
    "Unexpected middleware order response: " .. orderResult
)