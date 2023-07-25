cmake_minimum_required(VERSION 3.16)

find_package(BISON 3.6.1 REQUIRED) 
find_package(FLEX 2.6.4 REQUIRED)

BISON_TARGET(MyParser ${CMAKE_CURRENT_SOURCE_DIR}/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(MyScanner ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)