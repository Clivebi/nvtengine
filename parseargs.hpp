#pragma once
#include <iostream>
#include <list>
#include <string>

#include "engine/exception.hpp"

struct Option {
    bool isRequired;
    std::string strShort;
    std::string strLong;
    std::string strHelp;
    std::string strValue;
    Option(const char* Short, const char* Long, const char* Help, bool required)
            : strShort(Short), strLong(Long), strHelp(Help), strValue(""), isRequired(required) {}
    std::string Show() {
        if (!isRequired) {
            return "\t" + strShort + " " + strLong + " <optional> \t: " + strHelp;
        }
        return "\t" + strShort + " " + strLong + "\t: " + strHelp;
    }
};

struct Command {
    std::string strCmd;
    std::string strHelp;
    std::list<Option> options;
    Command(std::string cmd, std::string help, std::list<Option> options)
            : strCmd(cmd), strHelp(help), options(options) {}

    std::string JoinOptions() {
        std::string result;
        for (auto iter : options) {
            result += iter.strShort;
        }
        return result;
    }
    bool IsValid() {
        for (auto iter : options) {
            if (iter.isRequired && iter.strValue.size() == 0) {
                return false;
            }
        }
        return true;
    }
};

class ParseArgs {
protected:
    Command mCmd;
    std::list<Command> mAvaliableCmds;
    // app  -c=xxxxx update
    // app  -c=xxxxx scan -host -ports
public:
    void PrintHelp(std::string AppName) {
        for (auto iter : mAvaliableCmds) {
            std::cout << AppName << " " << iter.strCmd << " -[" << iter.JoinOptions() << "]"
                      << std::endl;
            std::cout << "options:" << std::endl;
            for (auto opt : iter.options) {
                std::cout << opt.Show() << std::endl;
            }
        }
    }

    explicit ParseArgs(int argc, char* argv[], std::list<Command> allCmds)
            : mCmd("", "", std::list<Option>()), mAvaliableCmds(allCmds) {
        if (argc < 2) {
            return;
        }
        for (auto iter : allCmds) {
            if (iter.strCmd == argv[1]) {
                mCmd = iter;
                break;
            }
        }
        if (mCmd.strCmd.size() == 0) {
            return;
        }
        for (int i = 2; i < argc; i++) {
            std::string str = argv[i];
            for (std::list<Option>::iterator iter = mCmd.options.begin();
                 iter != mCmd.options.end(); iter++) {
                if (str == iter->strShort || str == iter->strLong || str.find(iter->strLong) == 0) {
                    if (str == iter->strShort || str == iter->strLong) {
                        if (argc < i + 1) {
                            throw Interpreter::RuntimeException("invalid option " + str);
                        }
                        iter->strValue = argv[i + 1];
                        i++;
                        break;
                    }
                    iter->strValue = str.substr(9);
                    break;
                }
            }
        }
    }

    bool IsValid() { return mCmd.strCmd.size() > 0 && mCmd.IsValid(); }

    std::string GetCommand() { return mCmd.strCmd; }

    std::string GetOption(std::string opt) {
        for (auto iter : mCmd.options) {
            if (iter.strShort == opt || iter.strLong == opt) {
                return iter.strValue;
            }
        }
        return "";
    }
};