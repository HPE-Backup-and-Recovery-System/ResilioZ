cmake_minimum_required(VERSION 3.6)
project(HPE_Backup_Recovery_System VERSION 0.1)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
# 1. `-O3`: Enable All Optimizations
# 2. `-Wall`: Enable All Warnings
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# Specify Non-Header Source Files that should be Compiled
file(GLOB_RECURSE CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

list(REMOVE_ITEM CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/scheduler_server.cpp"
    "${PROJECT_SOURCE_DIR}/src/scheduler_client.cpp"
)

# Include Header Files
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Add Executables (Uses Globbing)
add_executable(
    scheduler_server src/scheduler_server.cpp ${CPP_SOURCE}
)

add_executable(
    scheduler_client src/scheduler_client.cpp ${CPP_SOURCE}
)

# Static Linking 
target_link_libraries(scheduler_server
    ${PROJECT_SOURCE_DIR}/libs/libcron/liblibcron.a
)

target_link_libraries(scheduler_client
    ${PROJECT_SOURCE_DIR}/libs/libcron/liblibcron.a
)
