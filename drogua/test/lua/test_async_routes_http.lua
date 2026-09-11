local function curl(path, method)
    local command = "curl -sS -f -X " .. method .. " http://127.0.0.1:18080" .. path
    local handle = assert(io.popen(command), "Failed to start curl")
    local result = handle:read("*a")
    local success, _, exitCode = handle:close()

    assert(success, "curl failed for " .. method .. " " .. path .. " (exit code: " .. tostring(exitCode) .. ")")
    return result
end

assert(curl("/lua/async/get", "GET") == '{"method":"GET"}')
assert(curl("/lua/async/post", "POST") == '{"method":"POST"}')
assert(curl("/lua/async/put", "PUT") == '{"method":"PUT"}')
assert(curl("/lua/async/delete", "DELETE") == '{"method":"DELETE"}')
assert(curl("/lua/async/patch", "PATCH") == '{"method":"PATCH"}')
assert(curl("/lua/async/request", "GET") == '{"method":"GET","path":"/lua/async/request"}')
