# Drogua

A Lua-powered REST API framework built on [Drogon](https://github.com/drogonframework/drogon) and [LuaBridge3](https://github.com/kunitoki/LuaBridge3), enabling developers to build high-performance REST APIs with Lua while leveraging Drogon's C++ HTTP and networking stack.

## Architecture

Drogua provides a Lua-facing API while keeping the underlying HTTP, networking, and database functionality in C++.

```text
Drogua
├── Lua
│   ├── Routes
│   ├── Request
│   ├── Response
│   ├── Database
│   └── Middleware
│
├── LuaBridge3
│   └── Lua ↔ C++ bindings
│
└── Drogon
    ├── HTTP server
    ├── Networking
    └── Database clients
```

## Documentation

Start with the [Application documentation](docs/app.md) to learn how to configure and run a Drogua application.

From there, you can explore:

* [Routes](docs/routes.md)
* [Request](docs/request.md)
* [Response](docs/response.md)
* [Database](docs/database.md)
* [Database Result](docs/database-result.md)
* [Database Row](docs/database-row.md)
* [Database Transactions](docs/database-transaction.md)
* [Middleware](docs/middleware.md)

## License

Drogua is licensed under the MIT License.

Drogua is built using the following open-source projects:

* [Drogon](https://github.com/drogonframework/drogon) — MIT License
* [Lua](https://www.lua.org/) — MIT License
* [LuaBridge3](https://github.com/kunitoki/LuaBridge3) — MIT License

The respective copyright notices and license terms of these projects remain applicable to their original code.
