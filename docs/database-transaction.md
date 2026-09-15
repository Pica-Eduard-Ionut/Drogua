# Database Transactions

`DatabaseTransaction` provides database transactions from Lua.

Create a synchronous transaction with `database:begin()`:

```lua
local db = Drogua.Database.get("default")
local tx = db:begin()
```

Create an asynchronous transaction with `database:beginAsync()`:

```lua
local db = Drogua.Database.get("default")
local tx = db:beginAsync()
```

Asynchronous transactions are intended for **async routes and middleware**.

---

## Basic Transaction

Execute multiple database operations and commit them together.

Synchronous:

```lua
local db = Drogua.Database.get("default")
local tx = db:begin()

tx:query([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Alice"})

tx:query([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Bob"})

local users = tx:query([[
    SELECT id, name
    FROM users
    ORDER BY id
]])

tx:commit()

return users:toTable()
```

Asynchronous transactions use `beginAsync()` and `queryAsync()`:

```lua
local db = Drogua.Database.get("default")
local tx = db:beginAsync()

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Alice"})

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Bob"})

local users = tx:queryAsync([[
    SELECT id, name
    FROM users
    ORDER BY id
]])

tx:commit()

return users:toTable()
```

---

## Parameters

Transactions support parameterized queries using a Lua table.

Synchronous:

```lua
local tx = db:begin()

local result = tx:query([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Alice"})

tx:commit()

return {
    affectedRows = result:affectedRows()
}
```

Asynchronous:

```lua
local tx = db:beginAsync()

local result = tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Alice"})

tx:commit()

return {
    affectedRows = result:affectedRows()
}
```

Parameters can be strings, integers, floating-point numbers, booleans, or `nil`.

```lua
local result = tx:queryAsync([[
    SELECT ? AS integer_value,
           ? AS double_value,
           ? AS enabled,
           ? AS empty_value
]], {
    42,
    3.14,
    true,
    nil
})
```

---

## Commit

Use `commit()` to finish a successful transaction.

```lua
local tx = db:beginAsync()

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], {"Charlie"})

tx:commit()

print(tx:valid()) -- false
```

After committing, the transaction is no longer active.

---

## Rollback

Use `rollback()` to explicitly discard the transaction.

```lua
local tx = db:beginAsync()

tx:queryAsync([[
    INSERT INTO users (name)
    VALUES (?)
]], {"ShouldNotExist"})

tx:rollback()

print(tx:valid()) -- false
```

---

## Automatic Rollback

If a transaction goes out of scope without being committed or explicitly rolled back, Drogua automatically rolls it back.

This applies to both synchronous and asynchronous transactions.

```lua
do
    local tx = db:beginAsync()

    tx:queryAsync([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"AutomaticRollback"})

    -- tx is destroyed here.
    -- The transaction is automatically rolled back.
end
```

This prevents an abandoned transaction from being accidentally committed.

---

## Checking Transaction State

Use `valid()` to check whether the transaction is still active.

```lua
local tx = db:beginAsync()

print(tx:valid()) -- true

tx:commit()

print(tx:valid()) -- false
```

The same `valid()` API is used for synchronous and asynchronous transactions.

---

## A Complete Example

A synchronous transaction can combine inserts, queries, parameters, and a final commit.

```lua
Routes.get("/users/create", function(req)
    local db = Drogua.Database.get("default")
    local tx = db:begin()

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Alice"})

    tx:query([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Bob"})

    local result = tx:query([[
        SELECT id, name
        FROM users
        WHERE name IN (?, ?)
        ORDER BY id
    ]], {"Alice", "Bob"})

    tx:commit()

    return result:toTable()
end)
```

The same transaction using the asynchronous API:

```lua
Routes.getAsync("/users/create", function(req)
    local db = Drogua.Database.get("default")
    local tx = db:beginAsync()

    tx:queryAsync([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Alice"})

    tx:queryAsync([[
        INSERT INTO users (name)
        VALUES (?)
    ]], {"Bob"})

    local result = tx:queryAsync([[
        SELECT id, name
        FROM users
        WHERE name IN (?, ?)
        ORDER BY id
    ]], {"Alice", "Bob"})

    tx:commit()

    return result:toTable()
end)
```

## API

| Method                    | Description                                     |
| ------------------------- | ----------------------------------------------- |
| `valid()`                 | Check whether the transaction is active         |
| `query(sql)`              | Execute a synchronous SQL query                 |
| `query(sql, params)`      | Execute a synchronous parameterized SQL query   |
| `queryAsync(sql)`         | Execute an asynchronous SQL query               |
| `queryAsync(sql, params)` | Execute an asynchronous parameterized SQL query |
| `executeAffected(sql)`    | Execute SQL and return affected rows            |
| `lastInsertId(sql)`       | Execute SQL and return the inserted ID          |
| `commit()`                | Commit the transaction                          |
| `rollback()`              | Roll back the transaction                       |

`begin()` creates a synchronous transaction, while `beginAsync()` creates an asynchronous transaction.

> Synchronous transactions use synchronous database operations and are intended for synchronous routes. Asynchronous transactions use `queryAsync()` and are intended for async routes and middleware.

---

Next: [Middleware](middleware.md)
