#pragma once

#include <iostream>

#define LEVEL_DEBUG 2
#define LEVEL_WARNING 1
#define LEVEL_ERROR 0

extern int g_LogLevel;

inline const char* LogLevelString(int level) {
    switch (level) {
    case 0:
        return "ERROR  ";
    case 1:
        return "WARN   ";
    default:
        return "DEBUG  ";
    }
}

inline std::ostream& LogLevelStream(int level) {
    switch (level) {
    case 0:
        return std::cerr;
    case 1:
        return std::cerr;
    default:
        return std::cout;
    }
}

template <typename T1>
void Log(int level, std::string where, int line, T1 msg) {
    if (level <= g_LogLevel) {
        LogLevelStream(level) << LogLevelString(level) << where << ":" << line << "\t" << msg
                              << std::endl;
    }
}
template <typename T1, typename T2>
void Log(int level, std::string where, int line, T1 msg, T2 a) {
    if (level <= g_LogLevel) {
        LogLevelStream(level) << LogLevelString(level) << where << ":" << line << "\t" << msg << a
                              << std::endl;
    }
}

template <typename T1, typename T2, typename T3>
void Log(int level, std::string where, int line, T1 msg, T2 a, T3 b) {
    if (level <= g_LogLevel) {
        LogLevelStream(level) << LogLevelString(level) << where << ":" << line << "\t" << msg << a
                              << b << std::endl;
    }
}

template <typename T1, typename T2, typename T3, typename T4>
void Log(int level, std::string where, int line, T1 msg, T2 a, T3 b, T4 c) {
    if (level <= g_LogLevel) {
        LogLevelStream(level) << LogLevelString(level) << where << ":" << line << "\t" << msg << a
                              << b << c << std::endl;
    }
}

//#define LOG(...) Log(__FUNCTION__, __LINE__, __VA_ARGS__)

#define LOG_ERROR(...) Log(LEVEL_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) Log(LEVEL_WARNING, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Log(LEVEL_DEBUG, __FUNCTION__, __LINE__, __VA_ARGS__)