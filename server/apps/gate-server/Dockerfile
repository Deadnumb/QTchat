FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

ARG APT_MIRROR=
RUN if [ -n "${APT_MIRROR}" ]; then         sed -i -e "s|http://archive.ubuntu.com/ubuntu/|${APT_MIRROR}|g"                -e "s|http://security.ubuntu.com/ubuntu/|${APT_MIRROR}|g"                /etc/apt/sources.list.d/ubuntu.sources 2>/dev/null || true;     fi

RUN apt-get update && apt-get install -y --no-install-recommends         build-essential cmake ninja-build git pkg-config ca-certificates         libboost-system-dev libboost-thread-dev libboost-filesystem-dev         libjsoncpp-dev libssl-dev         libprotobuf-dev protobuf-compiler         libgrpc++-dev protobuf-compiler-grpc         libmysqlcppconn-dev         libhiredis-dev     && rm -rf /var/lib/apt/lists/*

ARG REDIS_PLUS_PLUS_VERSION=1.3.15
ARG REDIS_PLUS_PLUS_REPO=https://github.com/sewenew/redis-plus-plus.git
RUN git clone --depth 1 -b ${REDIS_PLUS_PLUS_VERSION}         ${REDIS_PLUS_PLUS_REPO} /tmp/redis-plus-plus     && cmake -S /tmp/redis-plus-plus -B /tmp/redis-plus-plus/build -G Ninja         -DCMAKE_BUILD_TYPE=Release         -DREDIS_PLUS_PLUS_CXX_STANDARD=20         -DCMAKE_INSTALL_PREFIX=/usr/local     && cmake --build /tmp/redis-plus-plus/build -j"$(nproc)"     && cmake --install /tmp/redis-plus-plus/build     && ldconfig     && rm -rf /tmp/redis-plus-plus

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja         -DCMAKE_BUILD_TYPE=Release         -DCMAKE_SKIP_RPATH=ON         -DREDIS_PLUS_PLUS_DIR=/usr/local/lib/cmake/redis++     && cmake --build build -j"$(nproc)"

WORKDIR /app
RUN cp /src/build/bin/GateServer /app/GateServer

EXPOSE 8080
CMD ["./GateServer"]
