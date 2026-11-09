#!/usr/bin/env bash

set -euo pipefail

# Root directory: where deploy.sh is located
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source directory containing the DroGua application
SOURCE_DIR="$ROOT_DIR/drogua"

# Arguments
if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <service-name> <host-port> <docker-port>"
    exit 1
fi

SERVICE_NAME="$1"
HOST_PORT="$2"
DOCKER_PORT="$3"

# Validate service name
if [[ ! "$SERVICE_NAME" =~ ^[a-zA-Z0-9][a-zA-Z0-9_.-]*$ ]]; then
    echo "Error: Invalid service name."
    exit 1
fi

# Validate ports
for PORT in "$HOST_PORT" "$DOCKER_PORT"; do
    if ! [[ "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
        echo "Error: Ports must be between 1 and 65535."
        exit 1
    fi
done

# Output directory
OUTPUT_DIR="$ROOT_DIR/$SERVICE_NAME"

# Prevent overwriting existing directories
if [[ -e "$OUTPUT_DIR" ]]; then
    echo "Error: '$OUTPUT_DIR' already exists."
    exit 1
fi

# Required files and directories
REQUIRED_FILES=(
    "$SOURCE_DIR/build/drogua"
    "$SOURCE_DIR/lib"
    "$SOURCE_DIR/config.json"
    "$ROOT_DIR/licenses"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [[ ! -e "$file" ]]; then
        echo "Error: Missing required file: $file"
        exit 1
    fi
done

echo "Preparing deployment: $SERVICE_NAME"

# Create deployment directory
mkdir -p "$OUTPUT_DIR/build"

# Copy files from drogua/
cp -a "$SOURCE_DIR/build/drogua" "$OUTPUT_DIR/build/"
cp -a "$SOURCE_DIR/lib" "$OUTPUT_DIR/"
cp -a "$SOURCE_DIR/config.json" "$OUTPUT_DIR/"

# Copy licenses from project root
cp -a "$ROOT_DIR/licenses" "$OUTPUT_DIR/"

# Generate app.lua with the specified Docker port
cat > "$OUTPUT_DIR/app.lua" <<EOF
Drogua.app()
    :setLogPath("./")
    :setLogLevel("WARN")
    :addListener("0.0.0.0", $DOCKER_PORT)
    :setThreadNum(1) -- change to the amount of cores your CPU has or keep it as 1
    :loadJsonConfig("config")

Drogua.print("This message is coming from app.lua")

-- this is an example route
Drogua.Routes.get("/example", function(req)
    return {
        message = "This is the example route working!"
    }
end)

Drogua.app():run()
EOF

# Generate Containerfile
cat > "$OUTPUT_DIR/Containerfile" <<EOF
FROM alpine:3.24.2

WORKDIR /app

COPY build/drogua /app/drogua
COPY lib/ /app/lib/
COPY app.lua /app/app.lua
COPY config.json /app/config.json
COPY licenses/ /app/licenses/

RUN chmod +x /app/drogua

EXPOSE $DOCKER_PORT

CMD ["/app/drogua"]
EOF

# Generate compose.yaml
cat > "$OUTPUT_DIR/compose.yaml" <<EOF
services:
  $SERVICE_NAME:
    build:
      context: .
      dockerfile: Containerfile
    container_name: $SERVICE_NAME
    ports:
      - "$HOST_PORT:$DOCKER_PORT"
    restart: unless-stopped
EOF

echo ""
echo "Deployment prepared successfully!"
echo ""
echo "Service:      $SERVICE_NAME"
echo "Host port:    $HOST_PORT"
echo "Docker port:  $DOCKER_PORT"
echo "Output:       $OUTPUT_DIR"
echo ""
echo "To start:"
echo "  cd $SERVICE_NAME && docker compose up -d --build"
