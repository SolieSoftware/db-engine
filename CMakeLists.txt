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
include_directories(${PROJECT_SOURCE_DIR}/src/storage)

# Storage library (disk_manager and future storage classes)
add_library(storage
    src/storage/disk/disk_manager.cpp
    src/storage/page/page.cpp
    src/storage/buffer/lru_replacer.cpp
    src/storage/buffer/buffer_pool_manager.cpp
    src/storage/table/table_heap.cpp
    src/storage/index/b_plus_tree.cpp
    src/storage/index/b_plus_tree_page.cpp
    src/storage/index/b_plus_tree_leaf_page.cpp
    src/storage/index/b_plus_tree_internal_page.cpp
)

# Test executable
add_executable(test_disk_manager
    tests/test_disk_manager.cpp
)
target_link_libraries(test_disk_manager storage)

# Page test
add_executable(test_page
    tests/test_page.cpp
)
target_link_libraries(test_page storage)

# LRU Replacer test
add_executable(test_lru_replacer
    tests/test_lru_replacer.cpp
    )
target_link_libraries(test_lru_replacer storage)

# Buffer Pool Manager test
add_executable(test_buffer_pool_manager
    tests/test_buffer_pool_manager.cpp
    )
target_link_libraries(test_buffer_pool_manager storage)

add_executable(test_table_heap
    tests/test_table_heap.cpp
    )
target_link_libraries(test_table_heap storage)

add_executable(test_b_plus_tree
    tests/test_b_plus_tree.cpp
    )
target_link_libraries(test_b_plus_tree storage)

# Optional: Main executable (when you create it later)
# add_executable(db_engine, src/main.cpp)
