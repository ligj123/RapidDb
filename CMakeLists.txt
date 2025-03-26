cmake_minimum_required(VERSION 3.16)
project(RapidDB VERSION 1.0.0 DESCRIPTION "The fast database in the world")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_definitions(-DLINUX_OS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  add_definitions(-DWINDOWS_OS)
endif()

if(CACHE_TRACE STREQUAL "on")
  add_definitions(-DCACHE_TRACE)
endif()

if(NOT LOG_WITH_FILE STREQUAL "off")
  add_definitions(-DLOG_WITH_FILE)
endif()

if(WITHOUT_BIN_LOG STREQUAL "on")
  add_definitions(-DWITHOUT_BIN_LOG)
endif()

# add_definitions(-DCACHE_TRACE -DDEBUG_WITHOUT_BIN_LOG)
set(CMAKE_EXPORT_COMPILECOMMANDS ON)
project(RapidDB)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(_STACK_DEPTH 15)

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
