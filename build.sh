#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/drogua"
LICENSES_DIR="${SCRIPT_DIR}/licenses"

IMAGE="localhost/drogon-alpine:latest"
CONTAINER_PROJECT="/drogon/app"

echo "==> Building Drogua application"
echo "Project: ${PROJECT_DIR}"
echo "Licenses: ${LICENSES_DIR}"
echo "Image:   ${IMAGE}"

podman run --rm -it \
    -v="${PROJECT_DIR}:${CONTAINER_PROJECT}:Z,U" \
    -v="${LICENSES_DIR}:${CONTAINER_PROJECT}/licenses:Z,U,ro" \
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

    echo "==> Licenses available:"
    find /drogon/app/licenses -maxdepth 2 -type f -print

    echo "==> Build complete"
'

# reset permissions
echo "==> Resetting file ownership"
USER_ID="$(id -u)"
GROUP_ID="$(id -g)"
sudo chown -R "${USER_ID}:${GROUP_ID}" "${PROJECT_DIR}"
echo "==> Ownership reset to ${USER_ID}:${GROUP_ID}"
echo "==> Done"
