## 为什么会有这个项目  

OpenVAS是现在开源中比较活跃的NVT扫描引擎，目前它大概拥有大于7W个脚本，而且还在不断升级中，但是OpenVAS是一个老旧的脚本引擎，拥有臃肿的代码架构，而且不能跨平台，Nesses具有很好的跨平台性，而且脚本语言更加完善，但是不开源，所以本项目便应运而生，本项目旨在实现一个能够最大限度兼容openVAS现有脚本的扫描引擎，同时能够实现更加实用，简洁的脚本语法的可跨平台的NVT脚本引擎。

## 与openvas的区别  

脚本语法区别:  

1. 去掉nasl里面一个些语法设计（可能性能有损失，但是会简化脚本学习成本）。  
2. nasl的动态数组是一个蹩脚的设计，这里使用更加现代化的map和array实现。  
3. 取缔命名参数和匿名参数混用的问题，要么全匿名，要么全命名，内置API全部匿名。  
4. nasl脚本通过专用工具NaslToOther转换成本引擎使用的脚本格式。7W多个脚本转换后需要手动修改100多处。  
5. 更加易学，现代的脚本语法。  
6. 更加可靠，完善易扩展的内置功能，包括json，http，crypto支持。  

API区别:  

1. 大部分API不再具有业务逻辑，都是业务无关的，是更加纯粹的API。  
2. windows平台上samba和原始WMI支持过于老旧过时，我们使用WinRM来替代。  
3. 大部分原OpenVas的C API改在nasl.sc脚本中实现，这样实现和NASL老脚本API保持兼容性，但是又不会限制c++部分的设计。  

架构上的区别:  

1. 跨平台，去掉了原来的多进程逻辑，改为多线程,fork固然好，但是换成Windows整个设计就废了。  
2. 减少了依赖（主要用于解决跨平台），去掉了一些老旧的依赖库,openvas用了好几个加密解密库。  
3. c++实现更好的代码可维护性  
4. 除了脚本管理相关接口保留（后续可能移除），其余部分c++代码几乎移除了所有业务相关的代码。  
5. 更加严格的脚本运行时检查，通过这个功能目前已经发现很多个nasl的脚本bug。  
6. 废除redis的引用，改为内部内存实现,实现了更为强大的脚本间数据共享，现在可以共享连接等系统资源。  

## 为什么要设计一个新的脚本语言？  

1. 期望脚本语法能够实现openvas 的nasl（1.0）和nesses的nasl（3.0）语法的超集，这样本脚本有能力执行openvas脚本转换过来的脚本和nesses脚本转换过来的脚本。
2. 希望实现一个更加现代化的脚本语法，使得学习和使用更加方便。

## 目标平台

MAC、windows、linux、unix
项目中大部分代码使用标准c++编写，很少依赖系统特性，能够在主流平台上编译和运行。  

## 编译  

编译工具：  
MACOS：cmake、xcode、golang、pkg-config  
Linux：cmake、gcc（g++）、golang、pkg-config  
Windows：VS2022  （windows不需要golang环境）

依赖库：
MACOS：zlib openssl sqlite3 libssh  libbrotlidec libkrb5-dev net-snmp re2  
Linux：libbrotli-dev libkrb5-dev libssh-dev libsqlite3-dev libssl-dev libsnmp-dev libre2-dev  
Window：zlib openssl sqlite3 libssh  libbrotlidec  net-snmp re2  

MacOS&linux运行时依赖pcap库，Windows运行时依赖winpcap库，编译时不需要。

编译（MacOS&linux）:  

```

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

```

在MacOS上支持静态编译：
静态编译  
将上述所有 .a 文件复制到 build\staic 目录  
将CmakeLists.txt 中 #SET(MAC_STATIC "TRUE")注释去掉  
再执行make  
目前依赖库中re2在MacOS上无.a文件，所以需要手动编译re2静态库  


windows 使用vs2022编译，解决方案位于vs目录，依赖如下一些库：  
zlib openssl sqlite3 libssh  libbrotlidec  net-snmp re2  
需要首先编译这些依赖库。  

使用re2是因为c++ STL的正则表达式在处理大文本的时候经常导致栈溢出崩溃。  

## 如何使用
目前转换过来的openvas的脚本（同步到2021年11月的脚本）位于：https://github.com/Clivebi/nvtscript 
修改配置文件etc/nvtengine.conf 中的相关路径即可  
首次使用需要更新脚本信息一次：  
nvtengine update  
并且导入所有脚本到扫描控制数据库：  
nvtengine import -n all -t ../etc/rule.txt  
扫描一个目标：  
nvtengine scan -h 192.168.3.46 -p 22,1022,3000 -c ../etc/nvtengine.conf -f all  
开启jsonrpc服务器，通过jsonrpc启停扫描任务：  
nvtengine daemon -a localhost -p 6755  

## 架构  
![整体架构](https://github.com/Clivebi/nvtengine/blob/main/doc/img/nvtengine.png)

## 开发路径
0.1.0 版本  
- [x] 语法实现  
- [x] 基础内置API实现  
- [x] 无依赖端口(syn arp ping)扫描  
- [x] 无依赖服务发现  
- [x] openvas基础(string,http) API  
- [x] openvas数据包构造API  
- [x] openvas WMI API  
- [x] openvas SSH API  
- [x] 配置选项实现
- [x] 凭据管理
- [x] 实现windows/linux/mac完整跨平台

0.1.2  版本  
- [x] vfs集成
- [ ] openvas所有API实现支持
- [ ] 安全性提升，数据加密，签名
- [x] 管理接口完善
- [x] 脚本序列化加速执行

0.1.3  版本  
- [ ] 增强主机指纹识别功能
