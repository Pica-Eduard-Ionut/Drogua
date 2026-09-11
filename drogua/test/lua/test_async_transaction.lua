local Database = Drogua.Database
local Routes = Drogua.Routes

local db = Database.get("lua_test_db")

assert(db ~= nil, "Database.get('lua_test_db') returned nil")
assert(db:valid(), "lua_test_db is not valid")

-- ---------------------------------------------------------
-- Prepare transaction test database
-- ---------------------------------------------------------

db:exec([[
    CREATE TABLE IF NOT EXISTS async_transaction_users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL
    )
]])

db:exec("DELETE FROM async_transaction_users")
db:exec("DELETE FROM sqlite_sequence WHERE name = 'async_transaction_users'")

db:exec([[
    INSERT INTO async_transaction_users (name)
    VALUES ('Alice')
]])

db:exec([[
    INSERT INTO async_transaction_users (name)
    VALUES ('Bob')
]])

-- ---------------------------------------------------------
-- Basic async transaction
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-transaction/begin", function(req)
    local database = Database.get("lua_test_db")
    local tx = database:beginAsync()

    assert(tx ~= nil, "Database:beginAsync() returned nil")
    assert(tx:valid(), "New async transaction should be valid")

    local result = tx:queryAsync([[
        SELECT id, name
        FROM async_transaction_users
        ORDER BY id
    ]])

    tx:commit()

    return {
        success = true,
        transactionValid = tx:valid(),
        rows = result:toTable()
    }
end)

-- ---------------------------------------------------------
-- Async rollback
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-transaction/rollback", function(req)
    local database = Database.get("lua_test_db")
    local tx = database:beginAsync()

    tx:queryAsync([[
        INSERT INTO async_transaction_users (name)
        VALUES ('ShouldRollback')
    ]])

    tx:rollback()

    local check = database:query([[
        SELECT id, name
        FROM async_transaction_users
        WHERE name = 'ShouldRollback'
    ]])

    return {
        success = true,
        transactionValid = tx:valid(),
        exists = check:count() > 0
    }
end)

-- ---------------------------------------------------------
-- Async commit
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-transaction/commit", function(req)
    local database = Database.get("lua_test_db")
    local tx = database:beginAsync()

    local result = tx:queryAsync([[
        INSERT INTO async_transaction_users (name)
        VALUES ('AsyncCommitted')
    ]])

    tx:commit()

    local check = database:query([[
        SELECT id, name
        FROM async_transaction_users
        WHERE name = 'AsyncCommitted'
    ]])

    return {
        success = true,
        transactionValid = tx:valid(),
        affectedRows = result:affectedRows(),
        exists = check:count() > 0
    }
end)

-- ---------------------------------------------------------
-- Multiple async queries
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-transaction/multiple", function(req)
    local database = Database.get("lua_test_db")
    local tx = database:beginAsync()

    local users = tx:queryAsync([[
        SELECT id, name
        FROM async_transaction_users
        ORDER BY id
    ]])

    local count = tx:queryAsync([[
        SELECT COUNT(*) AS total
        FROM async_transaction_users
    ]])

    tx:commit()

    return {
        success = true,
        transactionValid = tx:valid(),
        users = users:toTable(),
        count = count:toTable()
    }
end)
