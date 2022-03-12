cmake_minimum_required(VERSION 3.16)
project (PressTest)

set(BOOST_ROOT C:/Boost)
set(BOOST_LIBRARYDIR C:/Boost/lib)
set(BOOST_INCLUDEDIR C:/Boost/include/boost-1_78)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.78.0 REQUIRED COMPONENTS log locale)
find_package (Threads)
#递归目录获取所有c文件
file(GLOB_RECURSE c_files *.cpp)

link_directories(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

add_executable(PressTest ${c_files})

target_link_libraries(PressTest
  PRIVATE
  rapid_static
    Boost::log
    Boost::locale
    ${CMAKE_THREAD_LIBS_INIT}
  )
