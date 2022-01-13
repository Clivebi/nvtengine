windows 编译指南  

1、编译zlib，libssh，openssl，brotli  
2、将相应头文件存放到D:\OpenCode\libs\include 目录下，lib库D:\OpenCode\libs\Win32\Release\ 和=D:\OpenCode\libs\x64\Release\  

3、下载sqlite3源码 头文件存放到上述位置，c文件存放到D:\OpenCode\libs\c\  

4、打开vs打开sln文件，编译相应版本即可  


build openssl  

perl Configure --prefix=D:\OpenCode\openssl\build --openssldir=D:\OpenCode\openssl\build\openssl --release --with-zlib-include=D:\OpenCode\libs\include\zlib --with-zlib-lib=D:\OpenCode\libs\Win32\Release\zlib.lib VC-WIN32  


perl Configure --prefix=D:\OpenCode\openssl\build --openssldir=D:\OpenCode\openssl\build\openssl --release --with-zlib-include=D:\OpenCode\libs\include\zlib --with-zlib-lib=D:\OpenCode\libs\x64\Release\zlib.lib VC-WIN64A  

使用MT编译openssl  
set CFLAGS="/MT"  
