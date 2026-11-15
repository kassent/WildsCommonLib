cmake_minimum_required(VERSION 3.21)
project(WildsCommonLib LANGUAGES C CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "WildsCommonLib requires Windows x64 and MSVC")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
# 收集common库的源文件
file(GLOB_RECURSE COMMON_SOURCES 
    "src/*.cpp"
    "src/*.c"
    "src/LightningScanner/*.cpp"
    "include/*.h"
    "include/*.hpp"
    "include/reframework/*.h"
    "include/reframework/*.hpp"

)

# 将lua文件添加到vs工程的lua文件夹中
file(GLOB_RECURSE LUA_SOURCES
    "include/lua/*.h"
    "include/lua/*.hpp"
    "src/lua/*.cpp"
    "src/lua/*.c"
)
source_group("lua" FILES ${LUA_SOURCES})


# 创建静态库
add_library(common_lib STATIC ${COMMON_SOURCES} ${LUA_SOURCES})
target_compile_features(common_lib PUBLIC cxx_std_23)

# 设置包含目录
target_include_directories(common_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/lua
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接公共依赖
target_link_libraries(common_lib
    PUBLIC
        spdlog::spdlog_header_only
        fmt::fmt
        nlohmann_json::nlohmann_json
        ZLIB::ZLIB
)

if(MSVC)
    # MSVC特定编译选项
    target_compile_options(common_lib PRIVATE 
        /wd4244 /wd4819 /wd4068 /utf-8
        $<$<CONFIG:Release>:/MT> 
        $<$<CONFIG:Debug>:/MTd>
    )
endif()

# 导出所有符号
set_target_properties(common_lib PROPERTIES 
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Explicit offline dispatch regression: no game process or deployment.
add_executable(CommonManagedVirtualCallTests EXCLUDE_FROM_ALL test/ManagedVirtualCallTests.cpp)
target_link_libraries(CommonManagedVirtualCallTests PRIVATE common_lib)
target_compile_definitions(CommonManagedVirtualCallTests PRIVATE NOMINMAX)
target_compile_options(CommonManagedVirtualCallTests PRIVATE /utf-8)

add_executable(CommonManagedAllocationTests EXCLUDE_FROM_ALL test/ManagedAllocationTests.cpp)
target_link_libraries(CommonManagedAllocationTests PRIVATE common_lib)
target_compile_definitions(CommonManagedAllocationTests PRIVATE NOMINMAX)
target_compile_options(CommonManagedAllocationTests PRIVATE /utf-8)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    add_test(NAME common.virtual-call COMMAND CommonManagedVirtualCallTests)
    add_test(NAME common.allocation COMMAND CommonManagedAllocationTests)
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
        DEPENDS CommonManagedVirtualCallTests CommonManagedAllocationTests)
endif()
