Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", 5555)
    :setThreadNum(2)
    :loadJsonConfig("config")
    --:enableRunAsDaemon()

Drogua.print("Hello from Lua!")
Drogua.print("This message is coming from app.lua")

-- DB test
Drogua.Routes.get("/test/db", function(req)

    -- Get it when the application is actually running.
    local db = Drogua.Database.get("default")

    Drogua.print("Database: " .. db:name())
    Drogua.print("Valid: " .. tostring(db:valid()))

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    for i = 0, users:count() - 1 do
        local row = users:row(i)

        Drogua.print(
            "id = " .. row:get("id")
        )

        Drogua.print(
            "name = " .. row:get("name")
        )
    end

    return {
        rows = users:count()
    }
end)

Drogua.Routes.get("/test/dbinser", function(req)

    local db = Drogua.Database.get("default")

    db:exec([[
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        )
    ]])

    db:exec([[
        INSERT INTO users (name)
        VALUES ('Alice')
    ]])

    db:exec([[
        INSERT INTO users (name)
        VALUES ('Bob')
    ]])

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    for i = 0, users:count() - 1 do
        local row = users:row(i)

        Drogua.print(row:toString())

        Drogua.print(
            "id=" .. row:get("id") ..
            " name=" .. row:get("name")
        )
    end

    return {
        message = "Database test completed",
        rows = users:count()
    }
end)

Drogua.Routes.get("/test/db/data", function(req)

    local db = Drogua.Database.get("default")

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    local data = {}

    for i = 0, users:count() - 1 do
        local row = users:row(i)

        table.insert(data, {
            id = row:get("id"),
            name = row:get("name")
        })
    end

    Drogua.print("Lua data type: " .. type(data))
    Drogua.print("Lua data length: " .. #data)
    Drogua.print("First user: " .. data[1].name)

    return data
end)

Drogua.Routes.get("/test/db/data/{id}", function(req, id)

    local db = Drogua.Database.get("default")

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    local data = {}

    --[[    This is one way to access the data ]]
    -- for i = 0, users:count() - 1 do
    --     local row = users:row(i)

    --     if row:get("id") == id then
    --         table.insert(data, {
    --             id = row:get("id"),
    --             name = row:get("name")
    --         })
    --     end
    -- end

    -- [[   This is an other way to access the data ]]
    -- for i = 0, users:count() - 1 do
    --     local row = users:row(i)

    --     if tonumber(row:get("id")) == tonumber(id) then
    --         table.insert(data, {
    --             id = row:get("id"),
    --             name = row:get("name")
    --         })
    --     end
    -- end

    -- [[ This is yet another way to access the data ]]
    -- for i = 0, users:count() - 1 do
    --     local row = users:row(i)

    --     if i == tonumber(id) - 1 then
    --         table.insert(data, {
    --             id = row:get("id"),
    --             name = row:get("name")
    --         })
    --     end
    -- end

    -- this tests the toTable function
    data = users:toTable()


    return data
end)

Drogua.app():run()

