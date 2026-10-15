cmake_minimum_required(VERSION 3.20)

project(StatusServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

add_executable(StatusServer
    src/main.cpp
    src/StatusServiceImpl.cpp
    "${CMAKE_SOURCE_DIR}/apps/gate-server/src/message.pb.cc"
    "${CMAKE_SOURCE_DIR}/apps/gate-server/src/message.grpc.pb.cc"
)

target_include_directories(StatusServer PRIVATE
    include
    "${CMAKE_SOURCE_DIR}/apps/gate-server/include"
)

target_link_libraries(StatusServer PRIVATE
    gRPC::grpc++
    protobuf::libprotobuf
)

set_target_properties(StatusServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/status"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.example.ini"
    "${CMAKE_BINARY_DIR}/status/config.ini"
    COPYONLY
)
