cmake_minimum_required(VERSION 3.16)
project(PressTest)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.74.0 REQUIRED COMPONENTS log locale stacktrace_backtrace)
find_package(Threads)
find_package(Backtrace)

# 递归目录获取所有c文件
file(GLOB_RECURSE c_files *.cpp)

link_directories(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

add_executable(PressTest ${c_files})

target_link_libraries(PressTest
  PRIVATE
  rapid_static
  Boost::log
  Boost::locale
  Boost::stacktrace_backtrace
  backtrace
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)
