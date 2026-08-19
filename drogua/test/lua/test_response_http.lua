local function curl(path, method)
    local command = "curl -sS -i -X " .. method .. " http://127.0.0.1:18080" .. path

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

local function assertContains(response, value)
    assert(
        response:find(value, 1, true) ~= nil,
        "Expected response to contain: " .. value ..
        "\nActual response:\n" .. response
    )
end


-- Basic response
local response = curl("/lua/response/basic", "GET")

assertContains(response, "HTTP/1.1 201")
assertContains(response, "Hello from LuaResponse!")


-- Headers and content type
response = curl("/lua/response/headers", "GET")

assertContains(response, "HTTP/1.1 202")
assertContains(response, "x-test: LuaResponse")
assertContains(response, "x-custom: hello")
assertContains(response, "content-type: text/plain")
assertContains(response, "Header test")


-- JSON response
response = curl("/lua/response/json", "GET")

assertContains(response, "HTTP/1.1 201")
assertContains(response, "content-type: application/json")
assertContains(response, "x-test: JSON")

assertContains(response, '"message" : "hello"')
assertContains(response, '"success" : true')
assertContains(response, '"number" : 123')


-- Nested JSON
response = curl("/lua/response/json-nested", "GET")

assertContains(response, "HTTP/1.1 202")
assertContains(response, "content-type: application/json")

assertContains(response, '"message" : "hello"')
assertContains(response, '"user"')
assertContains(response, '"id" : 42')
assertContains(response, '"name" : "Marian"')
assertContains(response, '"active" : true')


-- JSON array
response = curl("/lua/response/json-array", "GET")

assertContains(response, "HTTP/1.1 200")
assertContains(response, "content-type: application/json")

assertContains(response, '"items"')
assertContains(response, '"one"')
assertContains(response, '"two"')
assertContains(response, '"three"')
