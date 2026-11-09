# Drogua

Drogua is a Lua-powered REST API framework built on Drogon and LuaBridge3. It provides a Lua-facing API for building HTTP applications while leveraging Drogon's C++ infrastructure for HTTP handling, networking, event-driven execution, and database connectivity.

The goal is to combine Lua's simplicity and flexibility with the performance and capabilities of an established C++ web framework, allowing developers to write application logic in Lua without having to implement the underlying server infrastructure.

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

Drogua exposes a Lua API over selected functionality provided by Drogon. Lua applications interact with Drogua's interfaces, while LuaBridge3 handles communication between Lua and the underlying C++ implementation.

Drogon provides the core infrastructure, including HTTP request handling, networking, event-loop execution, and database connectivity.

The main Lua-facing components are:

* **Application** — application configuration and server startup.
* **Routes** — HTTP route registration and handlers.
* **Request** — access to incoming request data.
* **HTTP Request** — outgoing HTTP requests and related functionality.
* **Response** — constructing and returning HTTP responses.
* **Middleware** — request/response processing around route handlers.
* **Database** — database connections, queries, results, rows, and transactions.

## Example
This example demonstrates how to initialize and start a Drogua application, configure the HTTP server, and register a simple GET endpoint that returns a JSON response.
```lua
Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", 5555)
    :setThreadNum(1)
    :loadJsonConfig("config")

Drogua.Routes.get("/hello", function(req)
    return {
        message = "Hello, world!"
    }
end)

Drogua.app():run()
```

## Documentation

The documentation covers Drogua's application lifecycle, HTTP APIs, middleware, and database interfaces.

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


## Build and Deployment
Drogua uses a containerized Alpine Linux build environment with Drogon and the required build dependencies. The build process also collects the shared libraries needed at runtime, allowing the application to run in a lightweight container without including the full build toolchain.

### 1. Build the build environment
From the repository root, build the container image using Podman:
```bash
podman build -t localhost/drogon-alpine:latest .
```
This step only needs to be repeated when the build environment or its dependencies change.

### 2. Build Drogua
Run the build script:
```bash
./build.sh
```

The script:
- Builds Drogua.
- Runs the test suite.
- Collects the required runtime shared libraries into `drogua/lib/`.
- Verifies the application's runtime dependencies.

### 3. Prepare a deployment
Use the deployment script to generate a standalone deployment directory:
```bash
./deploy.sh <service-name> <host-port> <container-port>
```

For example:
```bash
./deploy.sh my-api 8080 8080
```

The script:
- Validates the service name and port numbers.
- Creates a deployment directory containing the compiled executable, runtime libraries, configuration, and license notices.
- Generates an example `app.lua` entry point configured with the selected container port.
- Generates a `Containerfile` and `compose.yaml` for container deployment.

Each deployment is generated in its own directory, allowing multiple Drogua applications to be prepared independently.

### 4. Run the deployment
Navigate to the generated deployment directory and start the service:
```bash
cd my-api
podman compose up -d --build
```

Although Podman is used for the build environment, the generated deployment uses the Compose specification and can be started with Docker Compose or a compatible Podman Compose setup.

## Benchmarks

Drogua was benchmarked against FastAPI using equivalent HTTP routes and workloads to compare request throughput, mean latency, and success rates under different target request rates.

### Test Environment

| Component | Configuration |
|---|---|
| CPU | Intel Core i7-3520M |
| RAM | 4 GB |
| Drogua | Drogon, 4 threads |
| FastAPI | Uvicorn, 4 workers |
| Benchmark Tool | PewPew 1.1.0 |
| Database | SQLite, 100 test rows |

Each endpoint was tested at target loads of **250, 750, and 1,500 RPS (requests per second)**, with three 10-second runs per configuration.

### Results

**`/sync` — HTTP handling and JSON response without database access**

| Target RPS | Drogua Avg RPS | FastAPI Avg RPS |
|---:|---:|---:|
| 250 | 274.04 | 270.48 |
| 750 | 765.71 | 756.02 |
| 1,500 | 968.85 | 772.98 |

**`/async` — HTTP handling with a SQLite query**

| Target RPS | Drogua Avg RPS | FastAPI Avg RPS |
|---:|---:|---:|
| 250 | 249.56 | 270.50 |
| 750 | 694.03 | 715.34 |
| 1,500 | 937.97 | 699.55 |

### Results Summary

At the 1,500 RPS target, Drogua achieved higher throughput than FastAPI on both endpoints:

- **`/sync`:** 968.85 RPS vs. 772.98 RPS (**25.3% higher throughput**).
- **`/async`:** 937.97 RPS vs. 699.55 RPS (**34.1% higher throughput**).

Drogua also recorded lower mean latency and higher success rates on both endpoints at this target load.

At lower target loads, throughput was generally comparable, with results varying by endpoint.

The measurements represent a specific hardware configuration, software setup, and workload. They should not be interpreted as absolute performance limits or universal comparisons between the frameworks.

For the complete benchmark methodology, latency measurements, success rates, route implementations, and framework configurations, see the [benchmark documentation](benchmark.md).

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
