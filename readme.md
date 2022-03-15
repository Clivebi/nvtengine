## 为什么会有这个项目  
OpenVAS是现在开源中比较活跃的NVT扫描引擎，目前它大概拥有大于7W个脚本，而且还在不断升级中，但是OpenVAS是一个老旧的脚本引擎，拥有臃肿的代码架构，而且不能跨平台，Nesses具有很好的跨平台性，而且脚本语言更加完善，所以本项目便应运而生，本项目旨在实现一个能够最大限度兼容openVAS现有脚本的扫描引擎，同时能够实现更加实用，简洁的脚本语法的可跨平台的NVT脚本引擎。

## 与openvas的区别  
脚本语法区别:  
1. 去掉nasl里面一个些语法设计（可能性能有损失，但是会简化脚本学习成本）。  
2. nasl的动态数组是一个蹩脚的设计，这里使用更加现代化的map和array实现。  
3. 取缔命名参数和匿名参数混用的问题，要么全匿名，要么全命名，内置API全部匿名。  
4. nasl脚本通过专用工具NaslToOther转换成本引擎使用的脚本格式。7W多个脚本转换后需要手动修改100多处。  
5. 更加易学，现代的脚本语法。  
6. 更加可靠，完善易扩展的内置功能，包括json，http，crypto支持。  

API区别:  
1. 大部分API不再具有业务逻辑，都是业务无关的，比如SSHConnect，内部不会去自动获取host，用户密码，这些参数，是更加纯粹的API。  
2. windows平台上samba和原始WMI支持过于老旧过时，我们使用WinRM来替代。  
3. 大部分API在一个脚本中实现，这样实现和NASL老脚本API保持兼容性，但是又不会限制c++部分的设计。  
架构上的区别:  
1. 跨平台，去掉了原来的多进程逻辑，改为多线程,fork固然好，但是换成Windows整个设计就废了。  
2. 减少了依赖（主要用于解决跨平台），去掉了一些老旧的依赖库  
3. c++实现更好的代码可维护性  
4. 除了脚本管理相关接口保留，其余部分c++代码几乎移除了所有业务相关的代码。  
5. 更加严格的脚本运行时检查，通过这个功能目前已经发现很多个nasl的脚本bug。  
6. 废除redis的引用，改为内部内存实现,实现了更为强大的脚本间数据共享，现在可以共享连接等系统资源。  

## 目标平台
MAC、windows、linux、unix
项目中大部分代码使用标准c++编写，很少依赖系统特性，能够在主流平台上编译和运行。  

## 依赖和编译  
安装gcc（xcode） go 开发环境  
MACOS:  
安装依赖 zlib openssl sqlite3 libssh  libbrotlidec libkrb5-dev net-snmp re2
mkdir build  
cd build  
cmake -DCMAKE_BUILD_TYPE=Release ..  
make   
静态编译  
将上述所有 .a 文件复制到 buil\staic 目录  
将CmakeLists.txt 中 #SET(MAC_STATIC "TRUE")注释去掉  
再执行make  


ubuntu:  
sudo apt install libbrotli-dev libkrb5-dev libssh-dev libsqlite3-dev libssl-dev libsnmp-dev libre2-dev  
mkdir build  
cd build  
cmake -DCMAKE_BUILD_TYPE=Release ..  
make 
ubuntu 不支持全静态编译~~  

运行时依赖pcap，编译时不需要  

windows 使用vs2022编译，解决方案位于vs目录，依赖如下一些库：  
zlib openssl sqlite3 libssh  libbrotlidec  net-snmp 
运行时依赖winpcap   

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
