#pragma once
#include <time.h>

#include <string>

#include "logger.hpp"
class FunctionPref {
private:
    clock_t mBegin;
    std::string mLocation;

public:
    explicit FunctionPref(const char* file, const char* func) {
        mLocation = file;
        mLocation += " ";
        mLocation += func;
        mBegin = clock();
    }
    ~FunctionPref() { NVT_LOG_DEBUG((clock() - mBegin), " clocks", mLocation); }
};

#define LOG_FUNC_PREF() FunctionPref(__FILE__,__FUNCTION__)