#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD_DIR="$ROOT_DIR/out/build/gate-server-debug"
BINARY="$BUILD_DIR/bin/GateServer"
REDIS_LIB_DIR=${REDIS_PLUS_PLUS_LIB_DIR:-"$HOME/Redis++/redis-plus-plus/build"}

if [ ! -x "$BINARY" ]; then
    "$SCRIPT_DIR/build-gate.sh"
fi

cd "$BUILD_DIR/bin"
export LD_LIBRARY_PATH="$REDIS_LIB_DIR:/usr/local/lib:${LD_LIBRARY_PATH:-}"
exec "$BINARY"
