cmake_minimum_required(VERSION 3.10)
project(RadixSortProject VERSION 1.0.0 DESCRIPTION "Radix Sort Implementation with Benchmarks")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译器选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# 查找TBB库
find_package(TBB REQUIRED)


# 设置包含目录
include_directories(include)

# 编译demo可执行文件
add_executable(demo demo/demo.cpp)

# 编译benchmark可执行文件
add_executable(benchmark benchmark/benchmark.cpp)

# 在你的目标中添加TBB链接
target_link_libraries(benchmark PRIVATE TBB::tbb)


# 可选：设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 可选：安装目标
# install(TARGETS demo benchmark
#     RUNTIME DESTINATION bin
# )
