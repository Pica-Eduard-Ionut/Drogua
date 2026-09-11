local Database = Drogua.Database
local Routes = Drogua.Routes

local db = Database.get("lua_test_db")

assert(db ~= nil, "Database.get('lua_test_db') returned nil")
assert(db:valid(), "lua_test_db is not valid")
assert(db:name() == "lua_test_db", "Unexpected database name")

-- ---------------------------------------------------------
-- Prepare test database
-- ---------------------------------------------------------

db:exec([[
    CREATE TABLE IF NOT EXISTS async_users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL
    )
]])

db:exec("DELETE FROM async_users")
db:exec("DELETE FROM sqlite_sequence WHERE name = 'async_users'")

db:exec([[
    INSERT INTO async_users (name)
    VALUES ('Alice')
]])

db:exec([[
    INSERT INTO async_users (name)
    VALUES ('Bob')
]])

-- ---------------------------------------------------------
-- GET /lua/async-database/query
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/query", function(req)
    local database = Database.get("lua_test_db")

    local users = database:queryAsync([[
        SELECT id, name
        FROM async_users
        ORDER BY id
    ]])

    return {
        count = users:count(),
        size = users:size(),
        columns = users:columns()
    }
end)

-- ---------------------------------------------------------
-- GET /lua/async-database/table
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/table", function(req)
    local database = Database.get("lua_test_db")

    local users = database:queryAsync([[
        SELECT id, name
        FROM async_users
        ORDER BY id
    ]])

    return users:toTable()
end)

-- ---------------------------------------------------------
-- GET /lua/async-database/params
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/params", function(req)
    local database = Database.get("lua_test_db")

    local users = database:queryAsync(
        "SELECT id, name FROM async_users WHERE id = ?",
        {1}
    )

    return users:toTable()
end)

-- ---------------------------------------------------------
-- GET /lua/async-database/multiple
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/multiple", function(req)
    local database = Database.get("lua_test_db")

    local first = database:queryAsync([[
        SELECT name
        FROM async_users
        WHERE id = 1
    ]])

    local second = database:queryAsync([[
        SELECT name
        FROM async_users
        WHERE id = 2
    ]])

    return {
        first = first:row(0):get("name"),
        second = second:row(0):get("name")
    }
end)

-- ---------------------------------------------------------
-- GET /lua/async-database/insert
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/insert", function(req)
    local database = Database.get("lua_test_db")

    local result = database:queryAsync([[
        INSERT INTO async_users (name)
        VALUES ('Charlie')
    ]])

    return {
        affectedRows = result:affectedRows(),
        insertId = result:insertId()
    }
end)

-- ---------------------------------------------------------
-- GET /lua/async-database/error
-- ---------------------------------------------------------

Routes.getAsync("/lua/async-database/error", function(req)
    local database = Database.get("lua_test_db")

    database:queryAsync([[
        SELECT *
        FROM table_that_does_not_exist
    ]])

    return { shouldNotReach = true }
end)
