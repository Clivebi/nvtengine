cmake_minimum_required(VERSION 3.12.0)
project(onescript)

#default build debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

SET(OUTDIR ${CMAKE_CURRENT_BINARY_DIR})

SET (PRODUCT_NAME "nvtstudio")
SET (PRODUCT_VERSION "0.1.1")
SET(INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
SET(DATA_DIR "${CMAKE_INSTALL_PREFIX}/var/${PRODUCT_NAME}")
SET(ECT_DIR "${CMAKE_INSTALL_PREFIX}/etc/${PRODUCT_NAME}")
SET(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin/")

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

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    execute_process (COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE MACSDK
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(ENV{SDKROOT} "${MACSDK}")
    message("build winrm.a on macos")
    execute_process (COMMAND go build -buildmode=c-archive -o ${OUTDIR}/winrm.a ./winrm.go
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/winrm 
        OUTPUT_VARIABLE WINRM_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE) 
ELSE()
    message("build winrm.a on linux")
    execute_process (COMMAND go build -buildmode=c-archive -o ${OUTDIR}/winrm.a ./winrm.go
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/winrm 
        OUTPUT_VARIABLE WINRM_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE)  
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")

message("${WINRM_RESULT}")

find_package(Threads REQUIRED)

pkg_check_modules (OPENSSL REQUIRED openssl>=1.1.0)
pkg_check_modules (ZLIB REQUIRED zlib>=1.2.11)
pkg_check_modules (SQLITE REQUIRED sqlite3>=3.0.0)
pkg_check_modules (SSH REQUIRED libssh>=0.9.0)
pkg_check_modules (RE2 re2)

find_library (SNMP netsnmp)
execute_process (COMMAND net-snmp-config --libs
    OUTPUT_VARIABLE SNMP_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

#https://github.com/google/brotli
pkg_check_modules (BROTLI REQUIRED libbrotlidec>=1.0.6)

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "openssl >=1.1.0 not found")
endif(NOT OPENSSL_FOUND)

if(NOT BROTLI_FOUND)
    message(FATAL_ERROR "libbrotlidec>=1.0.9 not found")
endif(NOT BROTLI_FOUND)

if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "zlib>=1.1.0 not found")
endif(NOT ZLIB_FOUND)

if (NOT SNMP_LDFLAGS)
    message(FATAL_ERROR "netsnmp>=5.8.0 not found")
endif(NOT SNMP_LDFLAGS)

message("${OPENSSL_LDFLAGS}")
message("${BROTLI_LDFLAGS}")
message("${ZLIB_LDFLAGS}")
message("${SQLITE_LDFLAGS}")
message("${SNMP_LDFLAGS}")
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                     ${ZLIB_INCLUDE_DIRS} 
                     ${BROTLI_INCLUDE_DIRS}
                     ${OPENSSL_INCLUDE_DIRS}
                     ${SNMP_INCLUDE_DIRS}
                     ${SSH_INCLUDE_DIRS}
                     ${RE2_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -std=c++17 -Werror")


add_definitions (-DPRODUCT_NAME="${PRODUCT_NAME}")
add_definitions (-DPRODUCT_VERSION="${PRODUCT_VERSION}")
add_subdirectory(engine)
add_subdirectory(thirdpart/masscan)

#SET(MAC_STATIC "TRUE")

if (MAC_STATIC)
    SET(STATIC_LIBS_DIR "${CMAKE_BINARY_DIR}/static")
    set(SSH_LDFLAGS "${STATIC_LIBS_DIR}/libssh.a")
    set(SQLITE_LDFLAGS "${STATIC_LIBS_DIR}/libsqlite3.a")
    set(ZLIB_LDFLAGS "${STATIC_LIBS_DIR}/libz.a")
    set(SNMP_LDFLAGS "${STATIC_LIBS_DIR}/libnetsnmp.a")
    set(OPENSSL_LDFLAGS "${STATIC_LIBS_DIR}/libssl.a;${STATIC_LIBS_DIR}/libcrypto.a")
    set(BROTLI_LDFLAGS "${STATIC_LIBS_DIR}/libbrotlidec-static.a;${STATIC_LIBS_DIR}/libbrotlicommon-static.a")
endif(MAC_STATIC)


set(WINRM_LDFLAGS "${CMAKE_BINARY_DIR}/winrm.a")

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
set(EXTENAL_LDFLAGS "-framework CoreFoundation -framework Security")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")

add_executable(nvtengine
    nvtengine.cc 
    taskmgr.cc
    modules/module.cc
    modules/openvas/api/snmpi.cc
    vfs/vfsreader.c
    manger.cc
)

add_executable(interpreter
    Interpreter.cc 
    modules/module.cc
    modules/openvas/api/snmpi.cc
)


message(${CMAKE_CURRENT_SOURCE_DIR})


add_custom_command(
        TARGET nvtengine
        COMMAND sed "-e" "'s|SCRIPT_DIR|${DATA_DIR}/nvtscript|g'" "-e" "'s|APP_DATA_DIR|${DATA_DIR}|g'" "-e" "'s|BUILTIN_SCRIPT|${DATA_DIR}/script|g'" "${CMAKE_BINARY_DIR}/../etc/nvtengine_in.conf" > "${CMAKE_BINARY_DIR}/../etc/nvtengine_src.conf"
        )


target_link_libraries(  nvtengine  
                        engine  
                        masscan 
                        -ldl 
                        ${WINRM_LDFLAGS}
                        "${KRB5_LDFLAGS}"
                        "-L${CMAKE_BINARY_DIR}"
                        ${EXTENAL_LDFLAGS}
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS} 
                        ${CMAKE_THREAD_LIBS_INIT}
                        ${SSH_LDFLAGS}
                        ${SNMP_LDFLAGS} 
                        ${RE2_LDFLAGS} 
                        )

target_link_libraries(  interpreter  
                        engine  
                        masscan 
                        -ldl
                        ${WINRM_LDFLAGS}
                        ${EXTENAL_LDFLAGS}
                        "${KRB5_LDFLAGS}"
                        "-L${CMAKE_BINARY_DIR}"
                        ${OPENSSL_LDFLAGS} 
                        ${ZLIB_LDFLAGS}
                        ${BROTLI_LDFLAGS}
                        ${SQLITE_LDFLAGS}
                        ${CMAKE_THREAD_LIBS_INIT}
                        ${SSH_LDFLAGS}
                        ${RE2_LDFLAGS} 
                        ${SNMP_LDFLAGS}  )


install (TARGETS interpreter
         RUNTIME DESTINATION ${BIN_DIR}
         PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
         GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install (TARGETS nvtengine
         RUNTIME DESTINATION ${BIN_DIR}
         PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
         GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install (DIRECTORY ${CMAKE_BINARY_DIR}/../script
         DESTINATION ${DATA_DIR})

install (FILES ${CMAKE_BINARY_DIR}/../etc/nvtengine_src.conf
         DESTINATION "${ECT_DIR}"
         RENAME "nvtengine.conf")