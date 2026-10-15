cmake_minimum_required(VERSION 3.20)

project(CPPChatProject LANGUAGES CXX)

# Keep the compilation database available for clangd and other language tools
# regardless of whether the project is configured through a preset, an IDE, or
# the development script.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

add_subdirectory(apps/gate-server)
add_subdirectory(apps/status-server)
