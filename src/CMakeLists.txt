cmake_minimum_required(VERSION 3.16)
project (rapid_static)

set(BOOST_ROOT C:/Boost)
set(BOOST_LIBRARYDIR C:/Boost/lib)
set(BOOST_INCLUDEDIR C:/Boost/include/boost-1_78)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.78.0 REQUIRED COMPONENTS log locale)
find_package (Threads)
#递归目录获取所有c文件
file(GLOB_RECURSE c_files *.cpp)

#生成动态库
add_library(rapid_static STATIC ${c_files})

#将动态库名字设为add
#set_target_properties(rapid_static PROPERTIES OUTPUT_NAME "rapid_static")
#set_target_properties(rapid_static PROPERTIES VERSION 1.0 SOVERSION 1)
