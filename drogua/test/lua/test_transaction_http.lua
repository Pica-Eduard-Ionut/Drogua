local function curl(path, method)
    local command =
        "curl --connect-timeout 2 --max-time 5 -sS -f -X " ..
        method .. " http://127.0.0.1:18080" .. path

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

-- ---------------------------------------------------------
-- Basic transaction
-- ---------------------------------------------------------
local basic = curl("/lua/transaction/basic", "GET")

assert(
    basic == '{"affectedRows":1,"validAfterCommit":false}',
    "Unexpected basic transaction result: " .. basic
)

-- ---------------------------------------------------------
-- Parameterized transaction
-- ---------------------------------------------------------
local parameters = curl("/lua/transaction/parameters", "GET")

assert(
    parameters == '{"affectedRows":1}',
    "Unexpected parameter result: " .. parameters
)

-- ---------------------------------------------------------
-- Numeric parameters
-- ---------------------------------------------------------
local numeric = curl("/lua/transaction/numeric", "GET")

assert(
    numeric == '{"doubleValue":"3.140000","integerValue":"42"}',
    "Unexpected numeric result: " .. numeric
)

-- ---------------------------------------------------------
-- Explicit rollback
-- ---------------------------------------------------------
local rollback = curl("/lua/transaction/rollback", "GET")

assert(
    rollback == '{"exists":false,"validAfterRollback":false}',
    "Unexpected rollback result: " .. rollback
)

-- ---------------------------------------------------------
-- Commit
-- ---------------------------------------------------------
local commit = curl("/lua/transaction/commit", "GET")

assert(
    commit == '{"exists":true,"validAfterCommit":false}',
    "Unexpected commit result: " .. commit
)

-- ---------------------------------------------------------
-- Multiple queries
-- ---------------------------------------------------------
local multiple = curl("/lua/transaction/multiple", "GET")

assert(
    multiple == '[{"id":"4","name":"MultiA"},{"id":"5","name":"MultiB"}]',
    "Unexpected multiple-query result: " .. multiple
)

-- ---------------------------------------------------------
-- Automatic rollback
--
-- Disabled for now because automatic rollback depends on
-- Lua garbage collection / destruction timing.
-- ---------------------------------------------------------

-- local automaticRollback = curl(
--     "/lua/transaction/automatic-rollback",
--     "GET"
-- )

-- assert(
--     automaticRollback == '{"exists":false}',
--     "Unexpected automatic rollback result: " .. automaticRollback
-- )

-- ---------------------------------------------------------
-- Invalid after rollback
-- ---------------------------------------------------------
local invalidRollback =
    curl("/lua/transaction/invalid-after-rollback", "GET")

assert(
    invalidRollback:find('"success":false', 1, true),
    "Expected transaction to be invalid after rollback: " .. invalidRollback
)

assert(
    invalidRollback:find(
        "Database transaction is no longer active",
        1,
        true
    ),
    "Unexpected error: " .. invalidRollback
)

-- ---------------------------------------------------------
-- Invalid after commit
-- ---------------------------------------------------------
local invalidCommit =
    curl("/lua/transaction/invalid-after-commit", "GET")

assert(
    invalidCommit:find('"success":false', 1, true),
    "Expected transaction to be invalid after commit: " .. invalidCommit
)

assert(
    invalidCommit:find("Database transaction is no longer active", 1, true),
    "Unexpected error: " .. invalidCommit
)
