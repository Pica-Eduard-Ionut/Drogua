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
        " (exit code: " .. tostring(exitCode) .. ")"
    )

    return result
end

assert(
    curl("/lua/get", "GET") == '{"method":"GET"}'
)

assert(
    curl("/lua/post", "POST") == '{"method":"POST"}'
)

assert(
    curl("/lua/put", "PUT") == '{"method":"PUT"}'
)

assert(
    curl("/lua/delete", "DELETE") == '{"method":"DELETE"}'
)

assert(
    curl("/lua/patch", "PATCH") == '{"method":"PATCH"}'
)

assert(
    curl("/lua/table", "GET") ==
    '{"active":true,"name":"Drogua","value":42}'
)

assert(
    curl("/lua/request", "GET") ==
    '{"method":"GET","path":"/lua/request"}'
)