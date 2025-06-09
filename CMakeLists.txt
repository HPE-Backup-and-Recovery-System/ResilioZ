cmake_minimum_required(VERSION 3.24...4.0.2)
project(HPE_Backup_Recovery_System VERSION 0.1)

# CPP Standard: C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile Options
# `-O3`: Enable All Optimizations
# `-Wall`: Enable All Warnings
# `-w`: Disable All Warnings
add_compile_options(-O3 -Wall)

# Other Required Options...
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# Specify Non-Header Source Files that should be Compiled
file(GLOB_RECURSE CPP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE BACKUP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/backup_restore/*.cpp"
)

file(GLOB_RECURSE SCHEDULER_SOURCE
    "${PROJECT_SOURCE_DIR}/src/schedulers/*.cpp"
)

# Exclude main & scheduler to Avoid Collision...
set(EXCLUDE_FILES
    "${PROJECT_SOURCE_DIR}/src/main.cpp"
    "${PROJECT_SOURCE_DIR}/src/scheduler.cpp"
    "${PROJECT_SOURCE_DIR}/src/backup-ui.cpp"
)

list(REMOVE_ITEM CPP_SOURCE ${EXCLUDE_FILES})

# Separating & Building Target utils (Common for Main and Scheduler)
file(GLOB_RECURSE UTILS_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/utils/*.cpp"
)
add_library(utils STATIC ${UTILS_SOURCE})

# Include Header Files
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Compile Definitions (Similar to #define - Dynamic)
add_compile_definitions(
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Add Executables (Uses Globbing)
add_executable(main src/main.cpp ${CPP_SOURCE})
add_executable(scheduler src/scheduler.cpp ${SCHEDULER_SOURCE})
add_executable(backup-ui src/backup-ui.cpp ${BACKUP_SOURCE})
# Dependencies...

# Find Packages which Contain FindLib.cmake - Install: libssh-dev, libzstd-dev, zlib1g-dev, pkg-config
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# To Find Packages without FindLib.cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSSH REQUIRED libssh) 
pkg_check_modules(ZSTD REQUIRED libzstd)

# Packages which can only be Installed via Git Repos or URLs
include(FetchContent)

FetchContent_Declare(
  libcron
  GIT_REPOSITORY https://github.com/PerMalmberg/libcron.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(libcron)
message(STATUS "Fetched Libcron...")

# Include Directories for Libraries in Main
target_include_directories(main PRIVATE 
    ${libcron_SOURCE_DIR}
    ${LIBSSH_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Include Directories for Libraries in Scheduler
target_include_directories(scheduler PRIVATE ${libcron_SOURCE_DIR})

target_include_directories(backup-ui PRIVATE 
    ${OPENSSL_INCLUDE_DIR}
    ${ZSTD_INCLUDE_DIRS}
)

# Link Libraries for Main
target_link_libraries(main PRIVATE
    utils
    libcron
    ${LIBSSH_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZSTD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Link Libraries for Scheduler
target_link_libraries(scheduler PRIVATE utils libcron)

target_link_libraries(backup-ui PRIVATE 
    utils
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${ZSTD_LIBRARIES}
)
