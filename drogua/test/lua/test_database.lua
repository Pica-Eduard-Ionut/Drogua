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
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL
    )
]])

-- Make the integration test deterministic.
db:exec("DELETE FROM users")

-- Reset SQLite AUTOINCREMENT so the first inserted row is id=1.
db:exec("DELETE FROM sqlite_sequence WHERE name = 'users'")

-- Insert known test data.
local insertAlice = db:exec([[
    INSERT INTO users (name)
    VALUES ('Alice')
]])

assert(insertAlice ~= nil, "Alice insert returned nil")

local insertBob = db:exec([[
    INSERT INTO users (name)
    VALUES ('Bob')
]])

assert(insertBob ~= nil, "Bob insert returned nil")

-- ---------------------------------------------------------
-- GET /lua/database/basic
-- ---------------------------------------------------------
Routes.get("/lua/database/basic", function(req)

    local database = Database.get("lua_test_db")

    return {
        name = database:name(),
        valid = database:valid()
    }

end)

-- ---------------------------------------------------------
-- GET /lua/database/query
-- ---------------------------------------------------------
Routes.get("/lua/database/query", function(req)

    local database = Database.get("lua_test_db")

    local users = database:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return {
        count = users:count(),
        size = users:size(),
        columns = users:columns(),
        firstColumn = users:columnName(0),
        secondColumn = users:columnName(1)
    }

end)

-- ---------------------------------------------------------
-- GET /lua/database/rows
-- ---------------------------------------------------------
Routes.get("/lua/database/rows", function(req)

    local database = Database.get("lua_test_db")

    local users = database:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    local first = users:row(0)
    local second = users:row(1)

    return {
        first = {
            id = first:get("id"),
            name = first:get("name"),
            value = first:toString()
        },

        second = {
            id = second:get("id"),
            name = second:get("name"),
            value = second:toString()
        }
    }

end)

-- ---------------------------------------------------------
-- GET /lua/database/table
-- ---------------------------------------------------------
Routes.get("/lua/database/table", function(req)

    local database = Database.get("lua_test_db")

    local users = database:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return users:toTable()

end)

-- ---------------------------------------------------------
-- GET /lua/database/insert
-- ---------------------------------------------------------
Routes.get("/lua/database/insert", function(req)

    local database = Database.get("lua_test_db")

    local result = database:exec([[
        INSERT INTO users (name)
        VALUES ('Charlie')
    ]])

    return {
        affectedRows = result:affectedRows(),
        insertId = result:insertId()
    }

end)

-- ---------------------------------------------------------
-- GET /lua/database/null
-- ---------------------------------------------------------
Routes.get("/lua/database/null", function(req)

    local database = Database.get("lua_test_db")

    database:exec([[
        CREATE TABLE IF NOT EXISTS nullable_test (
            id INTEGER,
            value TEXT
        )
    ]])

    database:exec("DELETE FROM nullable_test")

    database:exec([[
        INSERT INTO nullable_test (id, value)
        VALUES (1, NULL)
    ]])

    local result = database:query([[
        SELECT id, value
        FROM nullable_test
    ]])

    local row = result:row(0)

    return {
        id = row:get("id"),
        value = row:get("value"),
        row = row:toString()
    }

end)