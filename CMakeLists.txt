# Minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Project name and version
project(db_engine VERSION 1.0 LANGUAGES CXX)

#Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Warnings
if(MSVC) 
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/src/include)

# Storage library (disk_manager and future storage classes)
add_library(storage
    src/storage/disk_manager.cpp
)

# Test executable
add_executable(test_disk_manager
    src/test_disk_manager.cpp
)

# Link the storage library to the test executable
target_link_libraries(test_disk_manager storage)

# Optional: Main executable (when you create it later)
# add_executable(db_engine, src/main.cpp)
