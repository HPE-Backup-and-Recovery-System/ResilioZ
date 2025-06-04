cmake_minimum_required(VERSION 3.5)
project(HPE_Backup_Recovery_System VERSION 0.1)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
# 1. `-O3`: Enable All Optimizations
# 2. `-Wall`: Enable All Warnings
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# Specify Non-Header Source Files that should be Compiled
file(GLOB_RECURSE SCHEDULER_CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/schedulers/*.cpp"
)

file(GLOB_RECURSE REPOSITORY_CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/repositories/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/services/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/utils/*.cpp"
)

# Compile Definitions (Similar to #define - Dynamic)
add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# Include Header Files
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Link Dependencies
link_directories(${PROJECT_SOURCE_DIR}/libs)

# Add Executables (Uses Globbing)
add_executable(main src/main.cpp ${REPOSITORY_CPP_SOURCE})

add_executable(
    scheduler_server src/scheduler_server.cpp ${SCHEDULER_CPP_SOURCE}
)

add_executable(
    scheduler_client src/scheduler_client.cpp ${SCHEDULER_CPP_SOURCE}
)

# Static Linking 
target_link_libraries(scheduler_server
    ${PROJECT_SOURCE_DIR}/libs/libcron/liblibcron.a
)

target_link_libraries(scheduler_client
    ${PROJECT_SOURCE_DIR}/libs/libcron/liblibcron.a
)

target_link_libraries(main
    ${PROJECT_SOURCE_DIR}/libs/libssh/libssh.a
    ${PROJECT_SOURCE_DIR}/libs/openssl/libssl.a
    ${PROJECT_SOURCE_DIR}/libs/openssl/libcrypto.a
    ${PROJECT_SOURCE_DIR}/libs/zlib/libz.a
)
