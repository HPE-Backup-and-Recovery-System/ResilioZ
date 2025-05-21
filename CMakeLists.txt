cmake_minimum_required(VERSION 3.14)
project(LinuxBackupAndRecovery VERSION 1.0)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
# 1. `-O3`: Enable All Optimizations
# 2. `-Wall`: Enable All Warnings
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# Find nlohmann_json package
find_package(nlohmann_json 3.10.5 REQUIRED)

# Include Header Files
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files for main executable
set(MAIN_SOURCES
    src/main.cpp
    src/backup/backup_engine.cpp
    src/backup/nfs_handler.cpp
    src/backup/backup_interface.cpp
    src/logger/logger.cpp
)

# Add main executable
add_executable(main ${MAIN_SOURCES})

# Link libraries
target_link_libraries(main
    PRIVATE
    nlohmann_json::nlohmann_json
    pthread
)

# Copy config.json to build directory
configure_file(${PROJECT_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)

# Install target
install(TARGETS main
    RUNTIME DESTINATION bin
)
