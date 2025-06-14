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
file(GLOB_RECURSE MAIN_SOURCE
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE BACKUP_SOURCE
    "${PROJECT_SOURCE_DIR}/src/backup_restore/*.cpp"
)
file(GLOB_RECURSE SCHEDULER_SOURCE
    "${PROJECT_SOURCE_DIR}/src/schedulers/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/backup_restore/*.cpp"
)

# Exclude All Files Under src/gui/
file(GLOB_RECURSE GUI_FILES "${PROJECT_SOURCE_DIR}/src/gui/*")
list(REMOVE_ITEM MAIN_SOURCE ${GUI_FILES})

# Exclude main & scheduler to Avoid Collision...
set(EXCLUDE_FILES
    "${PROJECT_SOURCE_DIR}/src/main_gui.cpp"
    "${PROJECT_SOURCE_DIR}/src/main_cli.cpp"
    "${PROJECT_SOURCE_DIR}/src/scheduler.cpp"
    "${PROJECT_SOURCE_DIR}/src/backup-ui.cpp"
)

list(REMOVE_ITEM MAIN_SOURCE ${EXCLUDE_FILES})

# Separating & Building Target utils (Common for Main and Scheduler)
file(GLOB_RECURSE UTILS_SOURCE 
    "${PROJECT_SOURCE_DIR}/src/utils/*.cpp"
)
add_library(utils STATIC ${UTILS_SOURCE})

# Linking libcron as dependency for utils (Can be removed if external cron string validation added)
target_link_libraries(utils PRIVATE libcron)
target_include_directories(utils PRIVATE ${libcron_SOURCE_DIR})

# Include Header Files
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Compile Definitions (Similar to #define - Dynamic)
add_compile_definitions(
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Add Executables (Uses Globbing)
add_executable(cli src/main_cli.cpp ${MAIN_SOURCE})
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

# QT (GUI): qt6-base-dev, qtcreator, libxkbcommon-dev
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(GUI_SOURCES
    src/main_gui.cpp
    ${PROJECT_SOURCE_DIR}/include/gui/main_window.h
    ${PROJECT_SOURCE_DIR}/include/gui/tabs/backup_tab.h
    ${PROJECT_SOURCE_DIR}/include/gui/tabs/restore_tab.h
    ${PROJECT_SOURCE_DIR}/include/gui/tabs/services_tab.h
    ${GUI_FILES}
    ${MAIN_SOURCE}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(gui
        MANUAL_FINALIZATION
        ${GUI_SOURCES}
    )
else()
    add_executable(gui
        ${GUI_SOURCES}
    )
endif()

set_target_properties(gui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Include Directories for Libraries in Main (CLI + GUI)
target_include_directories(gui PRIVATE 
    ${libcron_SOURCE_DIR}
    ${LIBSSH_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(cli PRIVATE 
    ${libcron_SOURCE_DIR}
    ${LIBSSH_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${ZSTD_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Include Directories for Libraries in Scheduler
target_include_directories(scheduler PRIVATE 
    ${libcron_SOURCE_DIR}
    ${ZSTD_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZSTD_INCLUDE_DIRS}
)

target_include_directories(backup-ui PRIVATE 
    ${OPENSSL_INCLUDE_DIR}
    ${ZSTD_INCLUDE_DIRS}
)

# Link Libraries for Main (CLI + GUI)
target_link_libraries(gui PRIVATE
    utils
    libcron
    ${LIBSSH_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZSTD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_link_libraries(cli PRIVATE
    utils
    libcron
    ${LIBSSH_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZSTD_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Link Libraries for Scheduler
target_link_libraries(scheduler PRIVATE 
    utils 
    libcron
    ${OPENSSL_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${ZSTD_LIBRARIES}
)

target_link_libraries(backup-ui PRIVATE 
    utils
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${ZSTD_LIBRARIES}
)

# Additional Dependencies for GUI
include(GNUInstallDirs)
install(TARGETS gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(gui)
endif()
