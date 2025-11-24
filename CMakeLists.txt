cmake_minimum_required(VERSION 3.10)
project(Nudge)

set(CMAKE_CXX_STANDARD 23)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include folders
include_directories(include)
include_directories(sqlite)

# Source files from src/
file(GLOB SOURCES "src/*.cpp")

# Add SQLite implementation
list(APPEND SOURCES "sqlite/sqlite3.c")

# Build executable
add_executable(Nudge ${SOURCES})

