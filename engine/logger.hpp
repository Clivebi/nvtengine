#pragma once

#include <iostream>

template <typename T1>
void Log(std::string where, int line, T1 msg) {
    std::cout << where << ":" << line << "\t" << msg << std::endl;
}
template <typename T1, typename T2>
void Log(std::string where, int line, T1 msg, T2 a) {
    std::cout << where << ":" << line << "\t" << msg << a << std::endl;
}

template <typename T1, typename T2, typename T3>
void Log(std::string where, int line, T1 msg, T2 a, T3 b) {
    std::cout << where << ":" << line << "\t" << msg << a << b << std::endl;
}

template <typename T1, typename T2, typename T3, typename T4>
void Log(std::string where, int line, T1 msg, T2 a, T3 b, T4 c) {
    std::cout << where << ":" << line << "\t" << msg << a << b << c << std::endl;
}

#define LOG(...) Log(__FUNCTION__, __LINE__, __VA_ARGS__)