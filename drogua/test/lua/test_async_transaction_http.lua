local function curl(path, method)
    local command = "curl --connect-timeout 2 --max-time 5 -sS -X " .. method .. " http://127.0.0.1:18080" .. path
    local handle = assert(io.popen(command), "Failed to start curl")
    local result = handle:read("*a")
    local success, _, exitCode = handle:close()

    assert(success, "curl failed for " .. method .. " " .. path .. " (exit code: " .. tostring(exitCode) .. "): " .. result)
    return result
end

-- ---------------------------------------------------------
-- Basic async transaction
-- ---------------------------------------------------------

local begin = curl("/lua/async-transaction/begin", "GET")

assert(begin == '{"rows":[{"id":"1","name":"Alice"},{"id":"2","name":"Bob"}],"success":true,"transactionValid":false}', "Unexpected async begin result: " .. begin)

-- ---------------------------------------------------------
-- Async rollback
-- ---------------------------------------------------------

local rollback = curl("/lua/async-transaction/rollback", "GET")

assert(rollback == '{"exists":false,"success":true,"transactionValid":false}', "Unexpected async rollback result: " .. rollback)

-- ---------------------------------------------------------
-- Async commit
-- ---------------------------------------------------------

local commit = curl("/lua/async-transaction/commit", "GET")

assert(commit == '{"affectedRows":1,"exists":true,"success":true,"transactionValid":false}', "Unexpected async commit result: " .. commit)

-- ---------------------------------------------------------
-- Multiple async queries
-- ---------------------------------------------------------

local multiple = curl("/lua/async-transaction/multiple", "GET")

assert(multiple == '{"count":[{"total":"3"}],"success":true,"transactionValid":false,"users":[{"id":"1","name":"Alice"},{"id":"2","name":"Bob"},{"id":"3","name":"AsyncCommitted"}]}', "Unexpected multiple-query result: " .. multiple)
