build openssl

perl Configure --prefix=D:\OpenCode\openssl\build --openssldir=D:\OpenCode\openssl\build\openssl --release --with-zlib-include=D:\OpenCode\libs\include\zlib --with-zlib-lib=D:\OpenCode\libs\Win32\Release\zlib.lib VC-WIN32


perl Configure --prefix=D:\OpenCode\openssl\build --openssldir=D:\OpenCode\openssl\build\openssl --release --with-zlib-include=D:\OpenCode\libs\include\zlib --with-zlib-lib=D:\OpenCode\libs\x64\Release\zlib.lib 
VC-WIN64A

set ZLIB_INCLUDE_DIR=D:\OpenCode\libs\include\zlib
set ZLIB_LIBRARY=D:\OpenCode\libs\Win32\Release\
set OPENSSL_ROOT_DIR=D:\OpenCode\openssl\build
set ZLIB_ROOT_DIR=D:\OpenCode\libs\include\zlib