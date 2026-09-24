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
    rm -rf /drogon/app/lib
    mkdir -p /drogon/app/lib

    echo "==> Building application"
    cmake -S . -B build
    cmake --build build -j$(nproc)

    echo "==> Running tests"
    ctest --test-dir build --output-on-failure

    echo "==> Copying runtime libraries"

    ldd ./build/drogua \
        | awk '\''
            /=>/ && $3 ~ /^\// {
                print $3
            }
            !/=>/ && $1 ~ /^\// {
                print $1
            }
        '\'' \
        | sort -u \
        | while read -r lib; do
            echo "    $lib"
            cp -L "$lib" /drogon/app/lib/
        done

    echo "==> Libraries copied:"
    ls -lh /drogon/app/lib

    echo "==> Checking dependencies"

    LD_LIBRARY_PATH=/drogon/app/lib \
        ldd ./build/drogua

    echo "==> Build complete"
'
