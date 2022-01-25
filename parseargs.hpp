#pragma once
#include <iostream>
#include <list>
#include <string>
#include "engine/exception.hpp"
class ParseArgs {
protected:
    std::string mConfig;
    std::string mHostList;
    std::string mPortList;
    std::string mOidFilter;
    std::list<std::string> mCmds;
    // app  -c=xxxxx update
    // app  -c=xxxxx scan -host -ports
public:
    void PrintHelp() {
        std::cout << "NVTEngine [update scan] options" << std::endl;
        std::cout << "command update \t:update nvti database" << std::endl;
        std::cout << "command scan \t:scan target" << std::endl;
        std::cout << "options:" << std::endl;
        std::cout << "\t"
                  << "-c "
                  << "--config"
                  << "\t: config file" << std::endl;
        std::cout << "\t"
                  << "-h "
                  << "--hosts"
                  << "\t: scan target hosts list,use in scan command" << std::endl;
        std::cout << "\t"
                  << "-p "
                  << "--ports"
                  << "\t: scan port list,use in scan command" << std::endl;
        std::cout << "\t"
                  << "-f "
                  << "--filter"
                  << "\t: oid list filter,use in scan command" << std::endl;
    }
    explicit ParseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string str = argv[i];
            if (str == "-c" || str == "--config" || str.find("--config") == 0) {
                if (str == "-c" || str == "--config") {
                    if (argc < i + 1) {
                      throw Interpreter::RuntimeException ("options -c not valid");
                    }
                    mConfig = argv[i + 1];
                    i++;
                    continue;
                }
                mConfig = str.substr(9);
                continue;
            }
            if (str == "-h" || str == "--hosts" || str.find("--hosts") == 0) {
                if (str == "-h" || str == "--hosts") {
                    if (argc < i + 1) {
                        throw Interpreter::RuntimeException("options -h not valid");
                    }
                    mHostList = argv[i + 1];
                    i++;
                    continue;
                }
                mHostList = str.substr(8);
                continue;
            }
            if (str == "-p" || str == "--ports" || str.find("--ports") == 0) {
                if (str == "-p" || str == "--ports") {
                    if (argc < i + 1) {
                        throw Interpreter::RuntimeException("options -p not valid");
                    }
                    mPortList = argv[i + 1];
                    i++;
                    continue;
                }
                mPortList = str.substr(8);
                continue;
            }
            if (str == "-f" || str == "--filter" || str.find("--filter") == 0) {
                if (str == "-f" || str == "--filter") {
                    if (argc < i + 1) {
                        throw Interpreter::RuntimeException("options -f not valid");
                    }
                    mOidFilter = argv[i + 1];
                    i++;
                    continue;
                }
                mOidFilter = str.substr(9);
                continue;
            }
            if (str.size() && str[0] != '-') {
                mCmds.push_back(str);
            }
        }
    }
    std::string GetConfigFile() { return mConfig; }
    std::string GetHostList() { return mHostList; }
    std::string GetPortList() { return mPortList; }
    std::string GetOIDFilter() { return mOidFilter; }
    bool IsHaveUpdateNVTDatabaseCommand() {
        for (auto iter : mCmds) {
            if (iter == "update") {
                return true;
            }
        }
        return false;
    }
    bool IsHaveScanCommand() {
        for (auto iter : mCmds) {
            if (iter == "scan") {
                return true;
            }
        }
        return false;
    }
};