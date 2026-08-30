# Application

The `Drogua.app()` API is used to create and configure a Drogua application.

## Basic Application

```lua
local app = Drogua.app()

app:addListener("127.0.0.1", 18080)

app:run()
```

---

## Configuration

The application can be configured before calling `run()`:

```lua
local app = Drogua.app()

app:loadJsonConfig("config.json")

-- Keep this at 1 or 2 for now.
-- The Lua API is currently fully synchronous.
app:setThreadNum(2)

app:addListener("127.0.0.1", 18080)

app:setLogPath("./logs")
app:setLogLevel(...)

-- Keep daemon mode disabled when running in a container.
app:enableRunAsDaemon(false)

app:run()
```

> **Threading:** Keep the thread count at `1` or `2` for now. The current Lua API is fully synchronous, so using a large number of threads does not provide the expected benefits and may introduce unnecessary concurrency.

> **Daemon mode:** Keep daemon mode disabled when running Drogua inside a container. Enabling daemon mode causes the application process to detach, which can make the container's main process exit immediately and cause the container to stop.

---

## Application Methods

| Method                           | Description                      |
| -------------------------------- | -------------------------------- |
| `Drogua.app()`                   | Create an application            |
| `app:loadJsonConfig(path)`       | Load a Drogon JSON configuration |
| `app:setThreadNum(number)`       | Set the number of server threads |
| `app:addListener(address, port)` | Add an HTTP listener             |
| `app:setLogPath(path)`           | Set the log path                 |
| `app:setLogLevel(level)`         | Set the log level                |
| `app:enableRunAsDaemon(enabled)` | Enable or disable daemon mode    |
| `app:run()`                      | Start the application            |

---

## Typical Application

```lua
local app = Drogua.app()

app:loadJsonConfig("config.json")

-- 1-2 threads recommended for the current synchronous API.
app:setThreadNum(2)

app:addListener("127.0.0.1", 18080)

app:setLogPath("./logs")
app:setLogLevel(...)

-- Do not enable daemon mode inside a container.
app:enableRunAsDaemon(false)

local Routes = Drogua.Routes

Routes.get("/", function(req)
    return {
        message = "Hello from Drogua"
    }
end)

Routes.get("/hello/{name}", function(req, name)
    return {
        message = "Hello " .. name
    }
end)

app:run()
```

The general application flow is:

```text
Drogua.app()
    ↓
Configure application
    ↓
Register routes
    ↓
app:run()
```

---

## `Drogua.app()`

Creates a new application instance.

```lua
local app = Drogua.app()
```

---

## `app:loadJsonConfig(path)`

Loads a Drogon JSON configuration file.

```lua
app:loadJsonConfig("config.json")
```

---

## `app:setThreadNum(number)`

Sets the number of server threads.

For now, use `1` or `2`:

```lua
app:setThreadNum(2)
```

The Lua API is currently fully synchronous.

---

## `app:addListener(address, port)`

Adds an HTTP listener.

```lua
app:addListener("127.0.0.1", 18080)
```

---

## `app:setLogPath(path)`

Sets the log path.

```lua
app:setLogPath("./logs")
```

---

## `app:setLogLevel(level)`

Sets the application log level.

```lua
app:setLogLevel(...)
```

The accepted `level` values depend on the values exposed by the Drogua binding.

---

## `app:enableRunAsDaemon(enabled)`

Enables or disables daemon mode.

```lua
app:enableRunAsDaemon(false)
```

For containerized deployments, keep this disabled:

```lua
app:enableRunAsDaemon(false)
```

When daemon mode is enabled, the application detaches from the parent process. In a container, this can cause the main process to exit, resulting in the container stopping immediately.

---

## `app:run()`

Starts the application.

```lua
app:run()
```

This should normally be the final application call.

---

## Next: [Routes](routes.md)
