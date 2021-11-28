cmake_minimum_required(VERSION 3.12.0)
project(onescript)

#default build debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)

include (FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
  message(FATAL_ERROR "pkg-config executable not found. Aborting.")
endif (NOT PKG_CONFIG_FOUND)

pkg_check_modules (OPENSSL REQUIRED openssl>=1.1.0)
pkg_check_modules (ZLIB REQUIRED zlib>=1.2.11)
pkg_check_modules (SQLITE REQUIRED sqlite3>=3.0.0)

#https://github.com/google/brotli
pkg_check_modules (BROTLI REQUIRED libbrotlidec>=1.0.9)

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "openssl >=1.1.0 not found")
endif(NOT OPENSSL_FOUND)

if(NOT BROTLI_FOUND)
    message(FATAL_ERROR "libbrotlidec>=1.0.9 not found")
endif(NOT BROTLI_FOUND)

if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib>=1.1.0 not found")
endif(NOT ZLIB_FOUND)

message("${OPENSSL_LINK_LIBRARIES}")
message("${BROTLI_LINK_LIBRARIES}")
message("${ZLIB_LINK_LIBRARIES}")
message("${SQLITE_LINK_LIBRARIES}")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                     ${ZLIB_INCLUDE_DIRS} 
                     ${BROTLI_INCLUDE_DIRS}
                     ${OPENSSL_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -std=c++17 -Werror")

#set(CXX_FLAGS_DEBUG "${CXX_GCC_FLAGS} -Werror -ggdb3 -O0 -gdwarf-2")
#set(CXX_FLAGS_RELEASE "${CXX_FLAGS_RELEASE} ${CXX_GCC_FLAGS} -O2 -fPIC -gdwarf-2 -DNDEBUG -Wall")

set(GENSRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)

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

add_subdirectory(engine)
add_subdirectory(thirdpart/masscan)

set(OPENSSL_LDFLAGS "libssl.a;libcrypto.a")
set(BROTLI_LDFLAGS "libbrotlidec-static.a;libbrotlicommon-static.a")
set(ZLIB_LDFLAGS "${CMAKE_BINARY_DIR}/libz.a")
set(SQLITE_LDFLAGS "${CMAKE_BINARY_DIR}/libsqlite3.a")

add_executable(NVTEngine
    nvtengine.cc 
    taskmgr.cc
    modules/module.cc
)

add_executable(Interpreter
    Interpreter.cc 
    modules/module.cc
)


target_link_libraries(  NVTEngine  
                        engine  
                        masscan 
                        "-L${CMAKE_BINARY_DIR}"
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS} )

target_link_libraries(  Interpreter  
                        engine  
                        masscan 
                        "-L${CMAKE_BINARY_DIR}"
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS} )
