cmake_minimum_required(VERSION 3.10)
project(RadixSortProject VERSION 1.0.0 DESCRIPTION "Radix Sort Implementation with Benchmarks")

# 如果没有定义 CMAKE_BUILD_TYPE，设置为 Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "Setting CMAKE_BUILD_TYPE to Release (default)")
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译器选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # 源文件为 UTF-8 编码（含中文注释），需告知 MSVC 按 UTF-8 解析；
    # 否则在非 UTF-8 代码页（如 GBK/936）下中文注释会被误解析导致编译错误。
    add_compile_options(/utf-8)
endif()

# 查找TBB库
find_package(TBB REQUIRED)


# 设置包含目录
include_directories(include)

# ========================================
# 生成 cpu_config.h 文件 (支持 Windows 和 Linux)
# ========================================

# 确保 include 目录存在
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 检测 CPU 缓存大小
set(L1_CACHE_SIZE "32768")  # 默认值 32KB
set(L2_CACHE_SIZE "262144") # 默认值 256KB

if(WIN32)
    # Windows: 使用 PowerShell 获取缓存信息
    execute_process(
        COMMAND powershell -Command "$l1 = (Get-CimInstance -ClassName Win32_CacheMemory | Where-Object {$_.Level -eq 1 -and $_.CacheType -eq 'Other'} | Select-Object -First 1 -ExpandProperty MaxCacheSize); $l2 = (Get-CimInstance -ClassName Win32_CacheMemory | Where-Object {$_.Level -eq 2} | Select-Object -First 1 -ExpandProperty MaxCacheSize); if ($l1) { Write-Output $l1 } else { Write-Output '32768' }; if ($l2) { Write-Output $l2 } else { Write-Output '262144' }"
        OUTPUT_VARIABLE CACHE_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # 解析输出 (两行: 第一行 L1, 第二行 L2)
    string(REPLACE "\r\n" "\n" CACHE_OUTPUT "${CACHE_OUTPUT}")
    separate_arguments(CACHE_LINES NATIVE_COMMAND "${CACHE_OUTPUT}")
    list(GET CACHE_LINES 0 L1_CACHE_SIZE)
    list(GET CACHE_LINES 1 L2_CACHE_SIZE)

elseif(UNIX)
    # Linux: 从 /sys 文件系统读取缓存信息
    # L1 数据缓存通常是 index0，L2 缓存通常是 index2
    if(EXISTS "/sys/devices/system/cpu/cpu0/cache/index0/size")
        file(READ "/sys/devices/system/cpu/cpu0/cache/index0/size" L1_CACHE_RAW)
        string(STRIP "${L1_CACHE_RAW}" L1_CACHE_RAW)
        # 转换为字节数 (例如 "32K" -> 32768)
        if(L1_CACHE_RAW MATCHES "([0-9]+)K")
            math(EXPR L1_CACHE_SIZE "${CMAKE_MATCH_1} * 1024")
        elseif(L1_CACHE_RAW MATCHES "([0-9]+)M")
            math(EXPR L1_CACHE_SIZE "${CMAKE_MATCH_1} * 1024 * 1024")
        else()
            set(L1_CACHE_SIZE "${L1_CACHE_RAW}")
        endif()
    endif()

    if(EXISTS "/sys/devices/system/cpu/cpu0/cache/index2/size")
        file(READ "/sys/devices/system/cpu/cpu0/cache/index2/size" L2_CACHE_RAW)
        string(STRIP "${L2_CACHE_RAW}" L2_CACHE_RAW)
        # 转换为字节数
        if(L2_CACHE_RAW MATCHES "([0-9]+)K")
            math(EXPR L2_CACHE_SIZE "${CMAKE_MATCH_1} * 1024")
        elseif(L2_CACHE_RAW MATCHES "([0-9]+)M")
            math(EXPR L2_CACHE_SIZE "${CMAKE_MATCH_1} * 1024 * 1024")
        else()
            set(L2_CACHE_SIZE "${L2_CACHE_RAW}")
        endif()
    endif()
endif()

# 生成 cpu_config.h 文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpu_config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cpu_config.h
    @ONLY
)

message(STATUS "CPU Cache Configuration:")
message(STATUS "  L1 Data Cache Size: ${L1_CACHE_SIZE} bytes")
message(STATUS "  L2 Cache Size: ${L2_CACHE_SIZE} bytes")

if (WIN32)
  add_compile_options(/Fa)  # 生成.asm文件
endif()

# 编译demo可执行文件
add_executable(demo demo/demo.cpp)

# 编译benchmark可执行文件
add_executable(benchmark benchmark/benchmark.cpp)

# 编译test_bit_utils可执行文件
add_executable(test_bit_utils demo/test_bit_utils.cpp)

# 编译test_count_for_radix可执行文件
add_executable(test_count_for_radix demo/test_count_for_radix.cpp)

# 在你的目标中添加TBB链接
target_link_libraries(benchmark PRIVATE TBB::tbb)


# 可选：设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 可选：安装目标
# install(TARGETS demo benchmark
#     RUNTIME DESTINATION bin
# )
