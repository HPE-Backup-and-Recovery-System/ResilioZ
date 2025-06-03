cmake_minimum_required(VERSION 3.10)
project(hpe_backup)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
# 1. `-O3`: Enable All Optimizations
# 2. `-Wall`: Enable All Warnings
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# Find required packages
find_package(nlohmann_json REQUIRED)

# Use pkg-config to find ZSTD
include(FindPkgConfig)
pkg_check_modules(ZSTD REQUIRED libzstd)

# Find OpenSSL headers and libraries
find_path(OPENSSL_INCLUDE_DIR
    NAMES openssl/ssl.h openssl/crypto.h
    PATHS /usr/include /usr/local/include
)

find_library(OPENSSL_SSL_LIBRARY
    NAMES ssl
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

find_library(OPENSSL_CRYPTO_LIBRARY
    NAMES crypto
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

if(NOT OPENSSL_INCLUDE_DIR OR NOT OPENSSL_SSL_LIBRARY OR NOT OPENSSL_CRYPTO_LIBRARY)
    message(FATAL_ERROR "OpenSSL not found!")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${ZSTD_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/logger/logger.cpp
    src/incremental_backup/file_scanner.cpp
    src/incremental_backup/metadata_manager.cpp
    src/incremental_backup/ui_handler.cpp
    src/incremental_backup/compression_handler.cpp
    src/incremental_backup/compression_policy.cpp
    src/incremental_backup/chunk_hasher.cpp
)

# Create executable with name 'main'
add_executable(main ${SOURCES})

# Link libraries
target_link_libraries(main
    PRIVATE
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    nlohmann_json::nlohmann_json
    ${ZSTD_LIBRARIES}
    pthread
)

# Set build directory
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
