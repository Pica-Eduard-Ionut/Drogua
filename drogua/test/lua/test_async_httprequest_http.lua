local function curl(path, method)
    local command =
        "curl --connect-timeout 2 --max-time 5 -sS -X " ..
        method ..
        " http://127.0.0.1:18080" ..
        path

    local handle = assert(io.popen(command), "Failed to start curl")
    local result = handle:read("*a")
    local success, _, exitCode = handle:close()

    assert(
        success,
        "curl failed for " .. method .. " " .. path ..
        " (exit code: " .. tostring(exitCode) .. "): " .. result
    )

    return result
end

local result

result = curl("/lua/async-httprequest/get", "GET")
assert(
    result == '{"body":"{\\"message\\":\\"GET request received\\",\\"method\\":\\"GET\\",\\"success\\":true,\\"user_agent\\":\\"DrogonClient\\"}","ok":true,"status":200}',
    "Unexpected async GET result: " .. result
)

result = curl("/lua/async-httprequest/json", "GET")
assert(
    result == '{"message":"GET request received","method":"GET","status":200,"success":true,"user_agent":"DrogonClient"}',
    "Unexpected async JSON result: " .. result
)

result = curl("/lua/async-httprequest/post", "GET")
assert(
    result == '{"ok":true,"received":{"nested":{"enabled":true},"operation":"async-test","values":[10,20,30]},"status":200}',
    "Unexpected async POST result: " .. result
)

result = curl("/lua/async-httprequest/raw", "GET")
assert(
    result == '{"content_type":"text/plain; charset=utf-8","ok":true,"raw_body":"Hello from async Lua HTTP","status":200}',
    "Unexpected async raw POST result: " .. result
)

result = curl("/lua/async-httprequest/headers", "GET")
assert(
    result == '{"ok":true,"status":200,"test_header":"hello","test_number":"123"}',
    "Unexpected async headers result: " .. result
)

local command =
    "curl --connect-timeout 2 --max-time 5 -sS " ..
    "http://127.0.0.1:18080/lua/async-httprequest/error"

local handle = assert(io.popen(command), "Failed to start curl")
result = handle:read("*a")
local success, _, exitCode = handle:close()

assert(
    success,
    "curl failed for async HTTP error (exit code: " ..
    tostring(exitCode) .. "): " .. result
)

assert(
    result == '{"error":"intentional test error","ok":false,"status":500}',
    "Unexpected async HTTP error result: " .. result
)

result = curl("/lua/async-httprequest/retry", "GET")
assert(
    result == '{"body":{"message":"GET request received","method":"GET","success":true,"user_agent":"DrogonClient"},"ok":true,"status":200}',
    "Unexpected async retry result: " .. result
)

result = curl("/lua/async-httprequest/custom-headers", "GET")
assert(
    result == '{"ok":true,"status":200,"test_header":"hello","test_number":"123"}',
    "Unexpected async custom-header result: " .. result
)