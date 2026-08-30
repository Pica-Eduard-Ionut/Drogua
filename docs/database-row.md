# Database Row

`DatabaseRow` represents a single row returned by a database query.

Rows are obtained from a `DatabaseResult` using `row(index)`.

```lua
local result = db:query([[
    SELECT id, name
    FROM users
    ORDER BY id
]])

local user = result:row(0)
```

## Getting Values

Use `get()` with a column name to retrieve its value.

```lua
local user = result:row(0)

print(user:get("id"))
print(user:get("name"))
```

Values returned by `get()` are strings. SQL `NULL` values are returned as an empty string.

You can also access columns by their zero-based index:

```lua
print(user:get(0))
print(user:get(1))
```

## Checking for NULL

Use `isNull()` before reading a value when `NULL` matters.

```lua
local result = db:query([[
    SELECT id, value
    FROM nullable_test
]])

local row = result:row(0)

if row:isNull("value") then
    print("value is NULL")
end
```

Indexes can also be used:

```lua
if row:isNull(1) then
    print("second column is NULL")
end
```

## Column Information

Use `size()` to get the number of columns and `columnName()` to retrieve their names.

```lua
local row = result:row(0)

print(row:size())

for i = 0, row:size() - 1 do
    print(i, row:columnName(i))
end
```

Column indexes are **zero-based**.

## String Representation

`toString()` returns a simple representation of the row.

```lua
local row = result:row(0)

print(row:toString())

-- {id=1, name=Alice}
```

`NULL` values are represented as `NULL`:

```lua
-- {id=1, value=NULL}
```

## API

| Method              | Description                               |
| ------------------- | ----------------------------------------- |
| `size()`            | Number of columns                         |
| `columnName(index)` | Get a column name                         |
| `isNull(column)`    | Check whether a named column is `NULL`    |
| `isNull(index)`     | Check whether a column is `NULL` by index |
| `get(column)`       | Get a column value by name                |
| `get(index)`        | Get a column value by index               |
| `toString()`        | String representation of the row          |

> `get()` currently returns database values as strings. `NULL` values are returned as an empty string.

Next: [Database Transactions](database-transactions.md)
