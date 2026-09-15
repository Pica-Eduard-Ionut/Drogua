# Database

Drogua uses Drogon's database clients. Database connections are configured in `config.json` and accessed from Lua through `Drogua.Database`.

Drogua supports both **synchronous and asynchronous** database operations.

## Configuration

Database clients are configured under `db_clients` in the Drogon configuration file.

For example, a SQLite database:

```json
{
    "db_clients": [
        {
            "name": "default",
            "rdbms": "sqlite3",
            "filename": "./test.db",
            "is_fast": false,
            "number_of_connections": 1,
            "timeout": -1.0
        }
    ]
}
```

Drogua uses the same database configuration format as Drogon.

The database `name` is used when retrieving the client from Lua:

```lua
local db = Drogua.Database.get("default")
```

---

## Getting a Database

```lua
local db = Drogua.Database.get("default")

assert(db:valid())
print(db:name())
```

`Database.get()` returns a `DatabaseClient`.

If the requested database client does not exist, an error is raised.

### DatabaseClient API

| Method                    | Description                                     |
| ------------------------- | ----------------------------------------------- |
| `name()`                  | Returns the configured database name            |
| `valid()`                 | Returns whether the database client is valid    |
| `exec(sql)`               | Executes SQL and returns a `DatabaseResult`     |
| `query(sql, params)`      | Executes SQL, optionally with parameters        |
| `executeAffected(sql)`    | Executes SQL and returns the affected row count |
| `lastInsertId(sql)`       | Executes SQL and returns the last insert ID     |
| `begin()`                 | Starts a synchronous database transaction       |
| `queryAsync(sql, params)` | Executes a query asynchronously                 |
| `beginAsync()`            | Starts an asynchronous database transaction     |

The `Async` methods are intended to be used from Drogua's asynchronous routes and middleware.

---

## Executing SQL

Use `exec()` for SQL statements such as creating tables, inserting data, or updating data.

```lua
local db = Drogua.Database.get("default")

local result = db:exec([[
    INSERT INTO users (name)
    VALUES ('Alice')
]])

print(result:affectedRows())
print(result:insertId())
```

For asynchronous queries, use `queryAsync()`:

```lua
local db = Drogua.Database.get("default")

local result = db:queryAsync([[
    INSERT INTO users (name)
    VALUES ('Alice')
]])

print(result:affectedRows())
```

Async database operations can be used from an async route:

```lua
Routes.getAsync("/users", function(req)
    local db = Drogua.Database.get("default")

    local result = db:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return result:toTable()
end)
```

---

## Queries

Use `query()` when you need to retrieve rows:

```lua
local db = Drogua.Database.get("default")

local users = db:query([[
    SELECT id, name
    FROM users
    ORDER BY id
]])

for i = 0, users:count() - 1 do
    local user = users:row(i)

    print(
        "id=" .. user:get("id") ..
        " name=" .. user:get("name")
    )
end
```

The asynchronous equivalent is `queryAsync()`:

```lua
local users = db:queryAsync([[
    SELECT id, name
    FROM users
    ORDER BY id
]])
```

See [Database Result](database-result.md) and [Database Row](database-row.md) for working with query results.

---

## Parameterized Queries

Both `query()` and `queryAsync()` accept an optional Lua table containing parameters for `?` placeholders.

Synchronous:

```lua
local db = Drogua.Database.get("default")

local users = db:query([[
    SELECT id, name
    FROM users
    WHERE name = ?
]], {
    "Alice"
})
```

Asynchronous:

```lua
local users = db:queryAsync([[
    SELECT id, name
    FROM users
    WHERE name = ?
]], {
    "Alice"
})
```

Multiple parameters are passed in the same order as the placeholders:

```lua
local result = db:queryAsync([[
    SELECT id, name
    FROM users
    WHERE name = ? AND id = ?
]], { "Alice", 1 })
```

Use parameterized queries when inserting values supplied by requests instead of concatenating them into SQL.

---

## Affected Rows and Insert ID

For cases where you only need the affected row count:

```lua
local affected = db:executeAffected([[
    UPDATE users
    SET name = 'Bob'
    WHERE id = 1
]])

print(affected)
```

To execute a statement and retrieve its last insert ID:

```lua
local id = db:lastInsertId([[
    INSERT INTO users (name)
    VALUES ('Charlie')
]])

print(id)
```

These operations are currently available through the synchronous database API.

---

## Transactions

Synchronous transactions are created with `begin()`:

```lua
local db = Drogua.Database.get("default")
local tx = db:begin()

tx:query([[
    INSERT INTO users (name)
    VALUES (?)
]], { "Alice" })

tx:query([[
    INSERT INTO users (name)
    VALUES (?)
]], { "Bob" })

tx:commit()
```

Asynchronous transactions are created with `beginAsync()`:

```lua
local db = Drogua.Database.get("default")
local tx = db:beginAsync()

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], { "Alice" })

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], { "Bob" })

tx:commit()
```

Async transactions use `queryAsync()` for database operations. Transactions can be committed with `commit()` or cancelled with `rollback()`.

See [Database Transactions](database-transaction.md).

---

## Lua Database Modules

You can also use Lua database modules if they are imported into the container and available to your Lua environment.

For example, a Lua database library can be used normally:

```lua
local sqlite3 = require("lsqlite3")
local db = sqlite3.open("./test.db")
```

However, Lua database modules will **not have the same performance characteristics** as Drogua's native database wrapper.

Drogua's database API is integrated directly with Drogon and is the recommended option for application database access.

For asynchronous routes, use Drogua's native asynchronous database API such as `queryAsync()` and `beginAsync()`.

---

## Complete Example

A synchronous route can use the synchronous database API:

```lua
local Database = Drogua.Database
local Routes = Drogua.Routes

Routes.get("/users", function(req)
    local db = Database.get("default")

    local users = db:query([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return users:toTable()
end)
```

An asynchronous route uses the corresponding async API:

```lua
Routes.getAsync("/users", function(req)
    local db = Database.get("default")

    local users = db:queryAsync([[
        SELECT id, name
        FROM users
        ORDER BY id
    ]])

    return users:toTable()
end)
```

The database connection itself is configured in `config.json`; Lua only needs the configured client name.

---

## Next: [Database Result](database-result.md)
