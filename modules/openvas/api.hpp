#pragma once
#include "ovacontext.hpp"
inline int GetInt(std::vector<Value>& args, int pos, int default_value) {
    if (pos < args.size()) {
        if (args[pos].IsInteger()) {
            return args[pos].Integer;
        }
    }
    return default_value;
}
