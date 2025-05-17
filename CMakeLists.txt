cmake_minimum_required(VERSION 3.6)
project(HPE_Backup_Recovery_System VERSION 0.1)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# Include Directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# Source Files
file(GLOB_RECURSE CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*/*.cpp"
)

# Add Executable
add_executable(main ${CPP_SOURCE})
