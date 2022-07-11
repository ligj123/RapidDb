cmake_minimum_required(VERSION 3.16)

# SET(CMAKE_C_COMPILER /usr/bin/clang)
# SET(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_EXPORT_COMPILECOMMANDS ON)
project(RapidDB)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_C_FLAGS "-ldl -lbacktrace")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -rdynamic")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# 打印信息
message(STATUS "build type:${CMAKE_BUILD_TYPE}")
message(STATUS "CXX_FLAGS = ${CMAKE_CXX_FLAGS} ")

# 设置库文件输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

# 递归目录获取所有h文件
# include_directories(${PROJECT_SOURCE_DIR}/src)
# include_directories(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(presstest)
