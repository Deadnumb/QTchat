#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
PROTO_FILE="$ROOT_DIR/contracts/proto/message.proto"

command -v protoc >/dev/null
command -v protoc-gen-go >/dev/null
command -v protoc-gen-go-grpc >/dev/null
command -v grpc_cpp_plugin >/dev/null

protoc -I "$ROOT_DIR/contracts/proto" \
    --go_out="$ROOT_DIR/apps/verify-server" \
    --go_opt=module=github.com/deadnumb/cppchat/apps/verify-server \
    --go-grpc_out="$ROOT_DIR/apps/verify-server" \
    --go-grpc_opt=module=github.com/deadnumb/cppchat/apps/verify-server \
    "$PROTO_FILE"

protoc -I "$ROOT_DIR/contracts/proto" \
    --cpp_out="$ROOT_DIR/apps/gate-server/include" \
    --grpc_out="$ROOT_DIR/apps/gate-server/include" \
    --plugin=protoc-gen-grpc="$(command -v grpc_cpp_plugin)" \
    "$PROTO_FILE"

mv "$ROOT_DIR/apps/gate-server/include/message.pb.cc" "$ROOT_DIR/apps/gate-server/src/message.pb.cc"
mv "$ROOT_DIR/apps/gate-server/include/message.grpc.pb.cc" "$ROOT_DIR/apps/gate-server/src/message.grpc.pb.cc"
