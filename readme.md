## 概况
本项目旨在实现一个跨平台，能够执行openvas的nasl的NVT扫描引擎。  
NASL脚本可以通过自动化转换工具(NaslToOther)转换后由本引擎执行，转换后的部分脚本需要人工进行部分语法修改（大部分是nasl设计不科学的部分或者脚本存在bug的部分，在77000多个脚本中大概需要进行100处小改动） 

## 与openvas的区别
1. 跨平台，去掉了原来的多进程逻辑，改为多线程
2. 减少了依赖（主要用于解决跨平台），去掉了一些老旧的依赖库
3. c++实现更好的代码可维护性  
4. c++代码大量减少了业务相关的代码，比如，比如API内置host这种，转由中间脚本nasl.sc实现

## 语法兼容性
1. 禁止了原nasl匿名参数和命名参数混用的的语法，这种混用的语法给后续维护人员造成了很大困扰，而且容易存在参数无法完整检查带来的bug，新语法通过默认参数+命名参数实现同等功能。
2. 增加了原生map，array，支持，更加完整的http，json支持。
3. 更加严格的脚本运行时检查，通过这个功能目前已经发现很多个nasl的脚本bug。

## 目标平台
MAC、windows、linux、unix
项目中大部分代码使用标准c++编写，很少依赖系统特性，能够在主流平台上编译和运行。

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
- [ ] 配置选项实现
- [ ] 凭据管理
- [ ] 实现windows/linux/mac完整跨平台

0.1.2  版本  
- [ ] vfs集成
- [ ] openvas所有API实现支持
- [ ] 安全性提升，数据加密，签名
- [ ] 管理接口完善

0.1.3  版本  
- [ ] 增强主机指纹识别功能
