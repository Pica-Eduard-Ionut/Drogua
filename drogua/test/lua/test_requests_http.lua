local function curl(path, method, extra)
    local command =
        "curl -sS -f -X " .. method ..
        (extra or "") ..
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
    curl("/lua/request/method", "GET") ==
    '{"method":"GET"}'
)

assert(
    curl("/lua/request/path", "GET") ==
    '{"path":"/lua/request/path"}'
)

assert(
    curl("/lua/request/query?name=Drogua", "GET") ==
    '{"name":"Drogua"}'
)

assert(
    curl("/lua/request/header", "GET", ' -H "X-Test: hello"') ==
    '{"value":"hello"}'
)

assert(
    curl("/lua/request/cookie", "GET", ' -b "session=abc123"') ==
    '{"value":"abc123"}'
)

assert(
    curl("/lua/request/body", "POST", ' -d "hello"') ==
    '{"body":"hello"}'
)

assert(
    curl(
        "/lua/request/json",
        "POST",
        ' -H "Content-Type: application/json" -d \'{"name":"Drogua","age":42}\''
    ) ==
    '{"age":42,"name":"Drogua"}'
)

assert(
    curl(
        "/lua/request/all?name=Drogua",
        "GET",
        ' -H "X-Test: hello" -b "session=abc123"'
    ) ==
    '{"cookie":"abc123","header":"hello","method":"GET","path":"/lua/request/all","query":"Drogua"}'
)