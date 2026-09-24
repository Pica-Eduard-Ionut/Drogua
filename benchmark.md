## Benchmark Results

The benchmark compares **Drogua** and **FastAPI** using the same HTTP routes and workload.

### Test Environment

- **CPU:** Intel Core i7-3520M
- **RAM:** 4 GB
- **Drogua:** Drogon with 4 threads
- **FastAPI:** Uvicorn with 4 workers
- **Benchmark tool:** PewPew 1.1.0
- **Benchmark duration:** 10 seconds per run
- **Repetitions:** 3 runs per target RPS
- **Target RPS:** 250, 750, 1500
- **Cooldown:** 3 seconds between runs
- **Database:** SQLite
- **Database setup:** 100 test rows initialized before benchmarking

The benchmark runs each endpoint at each target RPS three times:

    2 implementations
    × 2 endpoints
    × 3 target RPS levels
    × 3 repetitions

The `/setup` request is performed before the benchmark and is **not included in the measured results**.

### Benchmark Command

Each request is benchmarked using PewPew 1.1.0 with a 10-second duration:

    ./pewpew benchmark \
        --rps "$target_rps" \
        --duration 10 \
        -t 5s \
        "${BASE_URL}/${endpoint}"

The benchmark records:

- Actual RPS
- Mean request latency
- Fastest request
- Slowest request
- Successful requests
- Failed requests
- Success percentage

### Results

#### `/sync`

| Target RPS | Drogua Avg RPS | Drogua Latency | Drogua Success | FastAPI Avg RPS | FastAPI Latency | FastAPI Success |
|---:|---:|---:|---:|---:|---:|---:|
| 250 | 274.04 | 80.66 ms | 100.00% | 270.48 | 141.66 ms | 100.00% |
| 750 | 765.71 | 303.66 ms | 100.00% | 756.02 | 434.33 ms | 100.00% |
| 1500 | 968.85 | 1253.17 ms | 91.56% | 772.98 | 1682.67 ms | 78.78% |

#### `/async`

| Target RPS | Drogua Avg RPS | Drogua Latency | Drogua Success | FastAPI Avg RPS | FastAPI Latency | FastAPI Success |
|---:|---:|---:|---:|---:|---:|---:|
| 250 | 249.56 | 231.83 ms | 100.00% | 270.50 | 162.83 ms | 100.00% |
| 750 | 694.03 | 693.50 ms | 98.96% | 715.34 | 1009.34 ms | 100.00% |
| 1500 | 937.97 | 1744.16 ms | 92.61% | 699.55 | 2672.33 ms | 77.27% |

### Results Summary

At the 250 RPS target, both implementations handled the `/sync` workload at approximately 270 RPS with a 100% success rate. For `/async`, FastAPI achieved higher measured throughput at this load, while Drogua had higher measured latency.

At 750 RPS, the measured throughput remained relatively close between the two implementations. Drogua recorded lower mean latency on both routes, while FastAPI maintained a 100% success rate on `/async` compared with 98.96% for Drogua.

At the 1500 RPS target, both implementations experienced increased latency and failed requests. Drogua recorded higher measured throughput and a higher success percentage than FastAPI on both `/sync` and `/async`.

These measurements describe this particular test environment and workload. They should not be interpreted as absolute performance limits for either framework. The test system has limited hardware, and the benchmark uses relatively short 10-second runs.

### Tested Routes

Both implementations expose the same routes:

| Method | Route | Description |
|---|---|---|
| `POST` | `/setup` | Initializes the SQLite database with 100 rows |
| `GET` | `/sync` | Returns a JSON response without database access |
| `GET` | `/async` | Reads one row from SQLite and returns its message |

The `/sync` and `/async` endpoints intentionally represent two different workloads:

- **`/sync`** — HTTP request handling and JSON serialization without database access.
- **`/async`** — HTTP request handling plus a SQLite query before generating the response.

The `/setup` request is performed before the benchmark and is **not included in the measured results**.

### Drogua Thread Configuration

The benchmark was run with Drogon configured for **4 threads**:

    Drogua.app()
        :setLogPath("./")
        :setLogLevel("WARN")
        :addListener("0.0.0.0", 5555)
        :setThreadNum(4)
        :loadJsonConfig("config")

The route implementations are the same regardless of the thread-count setting; `setThreadNum()` only changes the Drogon worker-thread configuration.

### Full Drogua Configuration
```lua
    Drogua.app()
        :setLogPath("./")
        :setLogLevel("WARN")
        :addListener("0.0.0.0", 5555)
        :setThreadNum(4)
        :loadJsonConfig("config")

    Drogua.Routes.post("/setup", function(req)

        local db = Drogua.Database.get("default")

        db:exec([[
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                message TEXT NOT NULL
            )
        ]])

        db:exec([[
            DELETE FROM messages
        ]])

        for i = 1, 100 do
            db:query([[
                INSERT INTO messages (message)
                VALUES (?)
            ]], {"Hello, World!"})
        end

        return {
            message = "Database initialized",
            entries = 100
        }
    end)

    Drogua.Routes.get("/sync", function(req)

        return {
            message = "Hello, World!"
        }

    end)

    Drogua.Routes.getAsync("/async", function(req)

        local db = Drogua.Database.get("default")

        local result = db:query([[
            SELECT message
            FROM messages
            WHERE id = 1
        ]])

        local rows = result:toTable()

        return {
            message = rows[1].message
        }

    end)

    Drogua.app():run()
```

### FastAPI Configuration
```python
    from contextlib import asynccontextmanager
    import aiosqlite
    from fastapi import FastAPI

    DB_PATH = "/app/test.db"
    app = FastAPI()

    @app.post("/setup")
    async def setup():
        async with aiosqlite.connect(DB_PATH) as db:
            await db.execute("""
                CREATE TABLE IF NOT EXISTS messages (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    message TEXT NOT NULL
                )
            """)

            await db.execute("DELETE FROM messages")
            await db.executemany(
                "INSERT INTO messages (message) VALUES (?)",
                [("Hello, World!",) for _ in range(100)],
            )

            await db.commit()

        return {
            "message": "Database initialized",
            "entries": 100,
        }

    @app.get("/sync")
    def sync_route():
        return {
            "message": "Hello, World!"
        }

    @app.get("/async")
    async def async_route():
        async with aiosqlite.connect(DB_PATH) as db:
            async with db.execute("""
                SELECT message
                FROM messages
                WHERE id = 1
            """) as cursor:
                row = await cursor.fetchone()

        return {
            "message": row[0]
        }
```