cmake_minimum_required(VERSION 3.16)
project(sql_parser)

FIND_PACKAGE(FLEX REQUIRED)
if (FLEX_FOUND)
    ADD_CUSTOM_TARGET(
        flex_target
        COMMAND ${FLEX_EXECUTABLE} 
            --header-file=${CMAKE_CURRENT_SOURCE_DIR}/../src/sql/flex_lexer.h
            --outfile=${CMAKE_CURRENT_SOURCE_DIR}/../src/sql/flex_lexer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/flex_lexer.l
        COMMENT "Generating flex_lexer.cpp"
    )
endif(FLEX_FOUND)

FIND_PACKAGE(BISON REQUIRED)
if (BISON_FOUND)
    ADD_CUSTOM_TARGET(
        bison_target
        COMMAND ${BISON_EXECUTABLE} 
            --defines=${CMAKE_CURRENT_SOURCE_DIR}/../src/sql/sql_parser.h
            --output=${CMAKE_CURRENT_SOURCE_DIR}/../src/sql/sql_parser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/sql_parser.y
        COMMENT "Generating sql_parser.cpp"
    )
endif(BISON_FOUND)