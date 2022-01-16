cmake_minimum_required(VERSION 3.12.0)

set(GENSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    OUTPUT ${GENSRC_DIR}/script.tab.hpp  ${GENSRC_DIR}/script.tab.cpp
    COMMAND bison --defines --output=${GENSRC_DIR}/script.tab.cpp ${CMAKE_CURRENT_SOURCE_DIR}/script.y
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/script.y)
set_source_files_properties(${GENSRC_DIR}/script.tab.cpp PROPERTIES GENERATED TRUE)

add_custom_command(
    OUTPUT ${GENSRC_DIR}/script.lex.cpp ${GENSRC_DIR}/script.lex.hpp
    COMMAND flex --header-file=${GENSRC_DIR}/script.lex.hpp --outfile=${GENSRC_DIR}/script.lex.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/script.l
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/script.l)
set_source_files_properties(${GENSRC_DIR}/script.lex.cpp  PROPERTIES GENERATED TRUE)

add_library(engine STATIC 
    parser.cc 
    script.tab.cpp
    script.lex.cpp 
    vm.cc
    value.cc
    vmcontext.cc 
    builtin.cc
    utf8.cc
    udobject.cc 
    tls.cc
)
