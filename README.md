# Drogua

A Lua-powered REST API framework built on [Drogon](https://github.com/drogonframework/drogon) and [LuaBridge3](https://github.com/kunitoki/LuaBridge3), enabling developers to build high-performance REST APIs with Lua while leveraging Drogon's C++ HTTP and networking stack.

## Architecture

Drogua provides a Lua-facing application framework while keeping the underlying HTTP, networking, and database functionality in C++.

```text
Drogua

├── Lua
│   ├── Application
│   ├── Routes
│   ├── Request
│   ├── HTTP Request
│   ├── Response
│   ├── Middleware
│   │
│   └── Database
│       ├── Queries
│       ├── Results
│       ├── Rows
│       └── Transactions
│
├── LuaBridge3
│   └── Lua <--> C++ bindings
│
└── C++
    └── Drogon
        ├── HTTP server
        ├── Networking
        ├── Event loop
        └── Database clients
```

Lua applications interact primarily with the Drogua API. LuaBridge3 connects those Lua APIs to the underlying C++ implementation, while Drogon provides the HTTP server, networking, asynchronous execution, and database connectivity.

The main Lua-facing components are:

* **Application** — application configuration and server startup.
* **Routes** — HTTP route registration and handlers.
* **Request** — access to incoming request data.
* **HTTP Request** — outgoing HTTP requests and related functionality.
* **Response** — constructing and returning HTTP responses.
* **Middleware** — request/response processing around route handlers.
* **Database** — database connections, queries, results, rows, and transactions.

## Documentation

Start with the [Application documentation](docs/app.md) to learn how to configure and run a Drogua application.

### API Documentation

* [Routes](docs/routes.md)
* [HTTP Request](docs/http-request.md)
* [Request](docs/request.md)
* [Response](docs/response.md)
* [Middleware](docs/middleware.md)

### Database Documentation

* [Database](docs/database.md)
* [Database Result](docs/database-result.md)
* [Database Row](docs/database-row.md)
* [Database Transactions](docs/database-transaction.md)

## Benchmarks

Benchmark results comparing Drogua and FastAPI under the same HTTP routes and workload are available in the [benchmark documentation](benchmark.md).

The benchmark includes:

* Test environment and hardware
* Benchmark methodology
* `/sync` and `/async` results
* Throughput and latency measurements
* Success and failure rates
* Drogon thread configuration
* Full Drogua configuration
* Equivalent FastAPI configuration

The benchmark results describe a specific test environment and workload and should not be interpreted as absolute performance limits for either framework.

## License

Drogua is licensed under the MIT License.

Drogua is built using the following open-source projects:

* [Drogon](https://github.com/drogonframework/drogon) — MIT License
* [Lua](https://www.lua.org/) — MIT License
* [LuaBridge3](https://github.com/kunitoki/LuaBridge3) — MIT License

Drogua also includes third-party runtime libraries and dependencies. The respective copyright notices and license terms of these projects remain applicable to their original code.

The third-party license texts and notices are provided in the [`/licenses`](licenses/) directory of this repository.

When using the Docker image, the same license texts and notices are available inside the container at:

```text
/app/licenses
```
