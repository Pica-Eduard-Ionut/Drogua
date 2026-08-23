local Database = Drogua.Database
local Routes = Drogua.Routes

local db = Database.get("lua_test_db")

assert(db ~= nil, "Database.get('lua_test_db') returned nil")
assert(db:valid(), "lua_test_db is not valid")

-- ---------------------------------------------------------
-- Prepare transaction test database
-- ---------------------------------------------------------
db:exec([[
    CREATE TABLE IF NOT EXISTS transaction_users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL
    )
]])

db:exec("DELETE FROM transaction_users")
db:exec("DELETE FROM sqlite_sequence WHERE name = 'transaction_users'")

-- ---------------------------------------------------------
-- Basic transaction
-- ---------------------------------------------------------
Routes.get("/lua/transaction/basic", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    assert(tx ~= nil, "Database:begin() returned nil")
    assert(tx:valid(), "New transaction should be valid")

    local result = tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"Alice"})

    assert(result ~= nil, "Transaction query returned nil")

    tx:commit()

    return {
        validAfterCommit = tx:valid(),
        affectedRows = result:affectedRows()
    }

end)

-- ---------------------------------------------------------
-- Parameterized transaction
-- ---------------------------------------------------------
Routes.get("/lua/transaction/parameters", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    local result = tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"Bob"})

    tx:commit()

    return {
        affectedRows = result:affectedRows()
    }

end)

-- ---------------------------------------------------------
-- Numeric parameters
-- ---------------------------------------------------------
Routes.get("/lua/transaction/numeric", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    local result = tx:query([[
        SELECT
            ? AS integer_value,
            ? AS double_value
    ]], {
        42,
        3.14
    })

    local row = result:row(0)

    tx:commit()

    return {
        integerValue = row:get("integer_value"),
        doubleValue = row:get("double_value")
    }

end)

-- ---------------------------------------------------------
-- Explicit rollback
-- ---------------------------------------------------------
Routes.get("/lua/transaction/rollback", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"ShouldNotExist"})

    tx:rollback()

    local check = database:query([[
        SELECT id, name
        FROM transaction_users
        WHERE name = ?
    ]], {"ShouldNotExist"})

    return {
        exists = check:count() > 0,
        validAfterRollback = tx:valid()
    }

end)

-- ---------------------------------------------------------
-- Commit
-- ---------------------------------------------------------
Routes.get("/lua/transaction/commit", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"Committed"})

    tx:commit()

    local check = database:query([[
        SELECT id, name
        FROM transaction_users
        WHERE name = ?
    ]], {"Committed"})

    return {
        exists = check:count() > 0,
        validAfterCommit = tx:valid()
    }

end)

-- ---------------------------------------------------------
-- Multiple queries in one transaction
-- ---------------------------------------------------------
Routes.get("/lua/transaction/multiple", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"MultiA"})

    tx:query([[
        INSERT INTO transaction_users (name)
        VALUES (?)
    ]], {"MultiB"})

    local result = tx:query([[
        SELECT id, name
        FROM transaction_users
        WHERE name IN (?, ?)
        ORDER BY id
    ]], {
        "MultiA",
        "MultiB"
    })

    tx:commit()

    return result:toTable()

end)

-- ---------------------------------------------------------
-- Automatic rollback
--
-- Transaction is not committed or explicitly rolled back.
-- LuaTransaction destructor should rollback it.
-- ---------------------------------------------------------
Routes.get("/lua/transaction/automatic-rollback", function(req)

    local database = Database.get("lua_test_db")

    do
        local tx = database:begin()

        tx:query([[
            INSERT INTO transaction_users (name)
            VALUES (?)
        ]], {"AutomaticRollback"})

        -- tx goes out of scope here.
        -- LuaTransaction destructor performs rollback.
    end

    local check = database:query([[
        SELECT id, name
        FROM transaction_users
        WHERE name = ?
    ]], {"AutomaticRollback"})

    return {
        exists = check:count() > 0
    }

end)

-- ---------------------------------------------------------
-- Invalid after rollback
-- ---------------------------------------------------------
Routes.get("/lua/transaction/invalid-after-rollback", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    tx:rollback()

    local success, errorMessage = pcall(function()
        tx:query("SELECT 1")
    end)

    return {
        success = success,
        error = errorMessage
    }

end)

-- ---------------------------------------------------------
-- Invalid after commit
-- ---------------------------------------------------------
Routes.get("/lua/transaction/invalid-after-commit", function(req)

    local database = Database.get("lua_test_db")
    local tx = database:begin()

    tx:commit()

    local success, errorMessage = pcall(function()
        tx:query("SELECT 1")
    end)

    return {
        success = success,
        error = errorMessage
    }

end)