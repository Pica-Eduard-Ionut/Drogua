Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", 5555)
    :setThreadNum(2)
    :loadJsonConfig("config")
    --:enableRunAsDaemon()

Drogua.print("Hello from Lua!")
Drogua.print("This message is coming from app.lua")


-- ============================================================
-- Create the table
-- ============================================================

Drogua.Routes.post("/test/db/setup", function(req)

    local db = Drogua.Database.get("default")

    db:exec([[
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        )
    ]])

    return {
        message = "Users table is ready"
    }
end)


-- ============================================================
-- Normal database INSERT
-- ============================================================

Drogua.Routes.post("/test/db/add", function(req)

    local db = Drogua.Database.get("default")

    local result = db:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Normal User"})

    return {
        message = "User inserted",
        affectedRows = result:affectedRows(),
        insertId = result:insertId()
    }
end)


-- ============================================================
-- Transaction test #1
--
-- Both INSERTs should be committed.
-- ============================================================

Drogua.Routes.post("/test/transaction/commit", function(req)

    local db = Drogua.Database.get("default")

    local tx = db:begin()

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Transaction Alice"})

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Transaction Bob"})

    tx:commit()

    return {
        message = "Transaction committed",
        expected = "2 users inserted"
    }
end)


-- ============================================================
-- Transaction test #2
--
-- The second query intentionally fails.
--
-- The first INSERT should therefore be rolled back as well.
-- ============================================================

Drogua.Routes.post("/test/transaction/rollback", function(req)

    local db = Drogua.Database.get("default")

    local tx = db:begin()

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Rollback Alice"})

    -- This intentionally fails because the column does not exist.
    --
    -- Drogon's transaction should automatically rollback.
    tx:query([[
        INSERT INTO users (this_column_does_not_exist)
        VALUES (?)
    ]], {"Rollback Bob"})

    -- This should never be reached because the query above throws.
    tx:commit()

    return {
        message = "This should never happen"
    }
end)


-- ============================================================
-- Transaction test #3
--
-- Explicit rollback.
-- ============================================================

Drogua.Routes.post("/test/transaction/explicit-rollback", function(req)

    local db = Drogua.Database.get("default")

    local tx = db:begin()

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Explicit Rollback User"})

    tx:rollback()

    return {
        message = "Transaction rolled back",
        expected = "user should NOT exist"
    }
end)


-- ============================================================
-- List all users
-- ============================================================

Drogua.Routes.get("/test/db/users", function(req)

    local db = Drogua.Database.get("default")

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return users:toTable()
end)


-- ============================================================
-- Transaction SELECT test
-- ============================================================

local auth = Drogua.Middleware.create(function(req, res, next)
    print("auth")
    next()
end)

local logging = Drogua.Middleware.create(function(req, res, next)
    print("logging")
    next()
end)

-- Drogua.Routes.get("/test/transaction/select", function(req)

--     local db = Drogua.Database.get("default")

--     local tx = db:begin()

--     local users = tx:query([[
--         SELECT id, name
--         FROM users
--         ORDER BY id
--     ]])

--     for i = 0, users:count() - 1 do
--         local row = users:row(i)

--         Drogua.print(
--             "id=" .. row:get("id") ..
--             " name=" .. row:get("name")
--         )
--     end

--     tx:commit()

--     return users:toTable()
-- end)

Drogua.Routes.get("/test/transaction/{id}/test/{att}", function(req, id, att)

    local db = Drogua.Database.get("default")

    local tx = db:begin()

    local users = tx:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    for i = 0, users:count() - 1 do
        local row = users:row(i)

        Drogua.print(
            "id=" .. row:get("id") ..
            " name=" .. row:get("name")
        )
    end

    tx:commit()

    return users:toTable()
end, { auth, logging })


Drogua.app():run()