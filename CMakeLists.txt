cmake_minimum_required(VERSION 3.6)
project(HPE_Backup_Recovery_System VERSION 0.1)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler Flags
# 1. `-O3`: Enable All Optimizations    
# 2. `-Wall`: Enable All Warnings
set(CMAKE_CXX_FLAGS "-O3 -Wall")

include_directories(${PROJECT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG v3.7.2
)

# Prevent libarchive from building its tests and tools
set(ENABLE_TEST OFF CACHE BOOL "Disable libarchive tests" FORCE)
set(ENABLE_COVERAGE OFF CACHE BOOL "Disable code coverage" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(ENABLE_UTIL OFF CACHE BOOL "Disable command-line utilities (bsdtar, etc)" FORCE)

FetchContent_MakeAvailable(libarchive)



# Specify Non-Header Source Files that should be Compiled
file(GLOB_RECURSE CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)
# Add Executables (Uses Globbing)
add_executable(main ${CPP_SOURCE})

target_link_libraries(main PRIVATE archive)
