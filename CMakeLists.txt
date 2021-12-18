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

find_program(KRB5 krb5-config)

if(KRB5-NOTFOUND)
    message(FATAL_ERROR "krb5-config executable not found. Aborting. you need install libkrb5-dev")
endif(KRB5-NOTFOUND)

execute_process (COMMAND krb5-config --libs gssapi
    OUTPUT_VARIABLE KRB5_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

pkg_check_modules (OPENSSL REQUIRED openssl>=1.1.0)
pkg_check_modules (ZLIB REQUIRED zlib>=1.2.11)
pkg_check_modules (SQLITE REQUIRED sqlite3>=3.0.0)
pkg_check_modules (SSH REQUIRED libssh>=0.9.0)
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

message("${OPENSSL_LIBRARY_DIRS}")
message("${BROTLI_LIBRARY_DIRS}")
message("${ZLIB_LIBRARY_DIRS}")
message("${SQLITE_LIBRARY_DIRS}")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                     ${ZLIB_INCLUDE_DIRS} 
                     ${BROTLI_INCLUDE_DIRS}
                     ${OPENSSL_INCLUDE_DIRS}
                     ${SSH_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -std=c++17 -Werror")


add_subdirectory(engine)
add_subdirectory(thirdpart/masscan)

SET(OUTDIR ${CMAKE_CURRENT_BINARY_DIR})



#this is for mac static 
set(OPENSSL_LDFLAGS "libssl.a;libcrypto.a")
set(BROTLI_LDFLAGS "libbrotlidec-static.a;libbrotlicommon-static.a")
set(ZLIB_LDFLAGS "${CMAKE_BINARY_DIR}/libz.a")
set(SQLITE_LDFLAGS "${CMAKE_BINARY_DIR}/libsqlite3.a")
set(SSH_LDFLAGS "${CMAKE_BINARY_DIR}/libssh.a")

set(WINRM_LDFLAGS "${CMAKE_BINARY_DIR}/winrm.a")
set(EXTENAL_LDFLAGS "-framework CoreFoundation -framework Security")

add_executable(NVTEngine
    nvtengine.cc 
    taskmgr.cc
    modules/module.cc
)

add_executable(Interpreter
    Interpreter.cc 
    modules/module.cc
)


message(${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(TARGET NVTEngine
    PRE_LINK
    COMMAND go build -buildmode=c-archive -o ${OUTDIR}/winrm.a ${CMAKE_CURRENT_SOURCE_DIR}/winrm/winrm.go 
    COMMAND echo "build winrm complete" 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/winrm)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/winrm/winrm.h  PROPERTIES GENERATED TRUE)


add_custom_command(TARGET Interpreter
    PRE_LINK
    COMMAND go build -buildmode=c-archive -o ${OUTDIR}/winrm.a ${CMAKE_CURRENT_SOURCE_DIR}/winrm/winrm.go 
    COMMAND echo "build winrm complete" 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/winrm)


target_link_libraries(  NVTEngine  
                        engine  
                        masscan 
                        ${WINRM_LDFLAGS}
                        "${KRB5_LDFLAGS}"
                        "-L${CMAKE_BINARY_DIR}"
                        ${EXTENAL_LDFLAGS}
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS} 
                        ${SSH_LDFLAGS})

target_link_libraries(  Interpreter  
                        engine  
                        masscan 
                        ${WINRM_LDFLAGS}
                        ${EXTENAL_LDFLAGS}
                        "${KRB5_LDFLAGS}"
                        "-L${CMAKE_BINARY_DIR}"
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS}
                        ${SSH_LDFLAGS} )
