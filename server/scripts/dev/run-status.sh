#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug}
BUILD_DIR="$ROOT_DIR/out/build/gate-server-debug"
if [ "$BUILD_TYPE" = "Release" ]; then
    BUILD_DIR="$ROOT_DIR/out/build/gate-server-release"
fi

cd "$BUILD_DIR/status"
exec ./StatusServer
