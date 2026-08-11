#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/drogua"

IMAGE="localhost/drogon-alpine:latest"
CONTAINER_PROJECT="/drogon/app"

echo "==> Building Drogua application"
echo "Project: ${PROJECT_DIR}"
echo "Image:   ${IMAGE}"

podman run --rm -it \
-v="${PROJECT_DIR}:${CONTAINER_PROJECT}:Z,U" \
-w="${CONTAINER_PROJECT}" \
"${IMAGE}" \
sh -lc '
set -e

    echo "==> Creating library directory"
    mkdir -p /drogon/app/lib

    echo "==> Building application"
    cmake -S . -B build
    cmake --build build -j$(nproc)

    echo "==> Copying runtime libraries"

    cp /usr/lib/libcares.so.2 /drogon/app/lib/
    cp /usr/lib/libjsoncpp.so.24 /drogon/app/lib/
    cp /lib/libuuid.so.1 /drogon/app/lib/
    cp /usr/lib/libpq.so.5 /drogon/app/lib/
    cp /usr/lib/libmariadb.so.3 /drogon/app/lib/
    cp /lib/libssl.so.1.1 /drogon/app/lib/
    cp /lib/libcrypto.so.1.1 /drogon/app/lib/
    cp /usr/lib/libsqlite3.so.0 /drogon/app/lib/
    cp /usr/lib/libhiredis.so.1.0.0 /drogon/app/lib/
    cp /lib/libz.so.1 /drogon/app/lib/
    cp /usr/lib/libstdc++.so.6 /drogon/app/lib/
    cp /usr/lib/libgcc_s.so.1 /drogon/app/lib/
    cp /usr/lib/libldap_r-2.4.so.2 /drogon/app/lib/
    cp /usr/lib/liblber-2.4.so.2 /drogon/app/lib/
    cp /usr/lib/libsasl2.so.3 /drogon/app/lib/

    # Lua 5.4
    cp -L /usr/lib/lua5.4/liblua-5.4.so.0 /drogon/app/lib/

    echo "==> Checking dependencies"

    LD_LIBRARY_PATH=/drogon/app/lib ldd ./build/drogua

    echo "==> Build complete"
'
