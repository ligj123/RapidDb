cmake_minimum_required(VERSION 3.16)
project(RapidClient)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.74.0 REQUIRED COMPONENTS log locale)
find_package(Threads)

# 递归目录获取所有c文件
file(GLOB_RECURSE c_files *.cpp)

link_directories(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

add_executable(RapidClient ${c_files})

target_link_libraries(RapidClient
  PRIVATE
  rapid_static
  Boost::log
  Boost::locale
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)
