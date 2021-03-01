cmake_minimum_required(VERSION 3.16)
project (UnitTest)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.75.0 REQUIRED COMPONENTS log locale unit_test_framework)
find_package (Threads)
#递归目录获取所有c文件
file(GLOB_RECURSE c_files *.cpp)

link_directories(${PROJECT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

add_executable(UnitTest ${c_files})

target_link_libraries(UnitTest
  PRIVATE
  rapid_static
    Boost::unit_test_framework
    Boost::log
    Boost::locale
    ${CMAKE_THREAD_LIBS_INIT}
  )

# avoid undefined reference to "main" in test.cpp
#target_compile_definitions(UnitTest
#  PRIVATE
#    BOOST_TEST_DYN_LINK
#  )

enable_testing()

add_test(
  NAME UnitTest
  COMMAND $<TARGET_FILE:${c_files}>
  )