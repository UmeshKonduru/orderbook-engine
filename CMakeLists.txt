cmake_minimum_required(VERSION 3.20)
project(OrderBookEngine VERSION 1.0.0 LANGUAGES CXX)

# Enforce modern C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Aggressive compiler optimization flags for HFT performance
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -O3
        -march=native
        -flto
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion
    )
    add_link_options(-flto)
endif()

# Specify where the compiler should look for header files
include_directories(include)

# Find system-installed GTest and Google Benchmark
find_package(GTest REQUIRED)
# find_package(benchmark REQUIRED) # Uncomment once benchmark files are created

# ---------------------------------------------------------
# 1. Core Engine Target (Static Library)
# ---------------------------------------------------------
add_library(orderbook_core STATIC
    # src/book/order_pool.cpp
    # src/book/order_lookup.cpp
    # src/book/order_book.cpp
    # src/book/price_level_lookups/flat_map.cpp
)

# ---------------------------------------------------------
# 2. Unit Tests Target
# ---------------------------------------------------------
enable_testing()

add_executable(unit_tests
    # tests/book/test_order_pool.cpp
    # tests/book/test_order_lookup.cpp
    # tests/book/test_order_book.cpp
    # tests/book/price_level_lookups/test_flat_map.cpp
)

# Link the core library and GTest to the test executable
target_link_libraries(unit_tests PRIVATE orderbook_core GTest::gtest_main)

# Register tests so they can be run with `ctest`
include(GoogleTest)
gtest_discover_tests(unit_tests)

# ---------------------------------------------------------
# 3. Performance Benchmark Targets
# ---------------------------------------------------------
# add_executable(bench_pool benchmarks/bench_pool.cpp)
# target_link_libraries(bench_pool PRIVATE orderbook_core benchmark::benchmark_main)

# add_executable(bench_book benchmarks/bench_book.cpp)
# target_link_libraries(bench_book PRIVATE orderbook_core benchmark::benchmark_main)
