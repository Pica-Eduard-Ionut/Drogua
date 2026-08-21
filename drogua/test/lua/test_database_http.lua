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

-- ---------------------------------------------------------
-- Basic database
-- ---------------------------------------------------------
assert(
    curl("/lua/database/basic", "GET") ==
    '{"name":"lua_test_db","valid":true}'
)

-- ---------------------------------------------------------
-- Result metadata
-- ---------------------------------------------------------
assert(
    curl("/lua/database/query", "GET") ==
    '{"columns":2,"count":2,"firstColumn":"id","secondColumn":"name","size":2}'
)

-- ---------------------------------------------------------
-- Row access
-- ---------------------------------------------------------
assert(
    curl("/lua/database/rows", "GET") ==
    '{"first":{"id":"1","name":"Alice","value":"{id=1, name=Alice}"},"second":{"id":"2","name":"Bob","value":"{id=2, name=Bob}"}}'
)

-- ---------------------------------------------------------
-- Result -> Lua table
-- ---------------------------------------------------------
assert(
    curl("/lua/database/table", "GET") ==
    '[{"id":"1","name":"Alice"},{"id":"2","name":"Bob"}]'
)

-- ---------------------------------------------------------
-- Insert
-- ---------------------------------------------------------
local insertResult =
    curl("/lua/database/insert", "GET")

assert(
    insertResult ==
    '{"affectedRows":1,"insertId":3}',
    "Unexpected insert result: " .. insertResult
)

-- ---------------------------------------------------------
-- NULL handling
-- ---------------------------------------------------------
assert(
    curl("/lua/database/null", "GET") ==
    '{"id":"1","row":"{id=1, value=NULL}","value":""}'
)