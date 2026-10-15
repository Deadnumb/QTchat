cmake_minimum_required(VERSION 3.20)

project(GateServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(REDIS_PLUS_PLUS_DIR "" CACHE PATH "Directory containing redis++-config.cmake")
if(NOT REDIS_PLUS_PLUS_DIR)
    foreach(candidate IN ITEMS
        "$ENV{HOME}/Redis++/redis-plus-plus/build/cmake"
        "/usr/local/lib/cmake/redis++"
    )
        if(EXISTS "${candidate}/redis++-config.cmake")
            set(REDIS_PLUS_PLUS_DIR "${candidate}" CACHE PATH
                "Directory containing redis++-config.cmake" FORCE)
            break()
        endif()
    endforeach()
endif()

if(REDIS_PLUS_PLUS_DIR)
    set(redis++_DIR "${REDIS_PLUS_PLUS_DIR}" CACHE PATH "redis++ package directory" FORCE)
endif()

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(mysql-concpp REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

set(GATE_SERVER_SOURCES
    src/AsioIOServicePool.cpp
    src/CServer.cpp
    src/ConfigManager.cpp
    src/GateServer.cpp
    src/HttpConnection.cpp
    src/LogicSystem.cpp
    src/MySQLDao.cpp
    src/MySQLManager.cpp
    src/RedisManager.cpp
    src/VerifyGrpcClient.cpp
    src/StatusGrpcClient.cpp
    src/message.grpc.pb.cc
    src/message.pb.cc
    src/utils.cpp
)

add_executable(GateServer ${GATE_SERVER_SOURCES})

target_include_directories(GateServer PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(GateServer PRIVATE
    Boost::system
    Boost::thread
    Boost::filesystem
    jsoncpp
    gRPC::grpc++
    protobuf::libprotobuf
    redis++::redis++
    mysql::concpp-jdbc
)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.example.ini"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.ini"
    COPYONLY
)

install(TARGETS GateServer RUNTIME DESTINATION bin)
