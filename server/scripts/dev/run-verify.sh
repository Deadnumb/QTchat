#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
OUT_DIR="$ROOT_DIR/out/bin"
CONFIG_FILE=${VERIFY_CONFIG_FILE:-}

if [ -z "$CONFIG_FILE" ]; then
    echo "请设置 VERIFY_CONFIG_FILE，指向仓库外的 VerifyServer 配置文件。" >&2
    exit 1
fi
if [ ! -f "$CONFIG_FILE" ]; then
    echo "VerifyServer 配置文件不存在：$CONFIG_FILE" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
cd "$ROOT_DIR/apps/verify-server"
go build -trimpath -o "$OUT_DIR/verify-server" .
cd "$ROOT_DIR"
VERIFY_CONFIG_FILE="$CONFIG_FILE" exec "$OUT_DIR/verify-server"
