#include <sstream>

#include "../engine/vm.hpp"

#ifndef _CHECK_HPP_
#define _CHECK_HPP_
inline std::string check_error(int i, const char* type, Interpreter::VMContext* ctx) {
    std::stringstream s;
    LOG(ctx->DumpContext(true));
    s << " : the #" << i << " argument must be an " << type << std::endl;
    return s.str();
}

inline std::string GetString(std::vector<Interpreter::Value>& args, int pos) {
    if (pos >= args.size()) {
        return "";
    }
    Interpreter::Value& arg = args[pos];
    return arg.ToString();
}

inline int GetInt(std::vector<Interpreter::Value>& args, int pos, int defaultVal = -1) {
    if (pos >= args.size()) {
        return defaultVal;
    }
    Interpreter::Value& arg = args[pos];
    return (int)arg.ToInteger();
}

#define ERROR_RETURN_NULL
#ifdef ERROR_RETURN_NULL
#define EXCEPTION(a) \
    LOG(a);          \
    return Value();
#else
#define EXCEPTION(a) throw RuntimeException(a)
#endif

#define CHECK_PARAMETER_COUNT(count)                                                    \
    if (args.size() < count) {                                                          \
        DEBUG_CONTEXT();                                                                \
        for (auto v : args) {                                                           \
            std::cerr << v.ToString() << std::endl;                                     \
        }                                                                               \
        EXCEPTION(std::string(__FUNCTION__) + " : the count of parameters not enough"); \
    }

#define CHECK_PARAMETER_RESOURCE(i)                                             \
    if (args[i].Type != ValueType::kResource) {                                 \
        for (auto v : args) {                                                   \
            std::cerr << v.ToString() << std::endl;                             \
        }                                                                       \
        EXCEPTION(std::string(__FUNCTION__) + check_error(i, "resource", ctx)); \
    }

#define CHECK_PARAMETER_STRING(i)                                                      \
    if (!args[i].IsStringOrBytes()) {                                                  \
        for (auto v : args) {                                                          \
            std::cerr << v.ToString() << std::endl;                                    \
        }                                                                              \
        EXCEPTION(std::string(__FUNCTION__) + check_error(i, "string or bytes", ctx)); \
    }
#define CHECK_PARAMETER_STRINGS()                                                          \
    for (size_t i = 0; i < args.size(); i++) {                                             \
        if (args[i].Type != ValueType::kBytes && args[i].Type != ValueType::kString) {     \
            for (auto v : args) {                                                          \
                std::cerr << v.ToString() << std::endl;                                    \
            }                                                                              \
            EXCEPTION(std::string(__FUNCTION__) + check_error(i, "string or bytes", ctx)); \
        }                                                                                  \
    }

#define CHECK_PARAMETER_INTEGER(i)                                             \
    if (!args[i].IsInteger()) {                                                \
        for (auto v : args) {                                                  \
            std::cerr << v.ToString() << std::endl;                            \
        }                                                                      \
        EXCEPTION(std::string(__FUNCTION__) + check_error(i, "integer", ctx)); \
    }

#define CHECK_PARAMETER_MAP(i)                                             \
    if (args[i].Type != ValueType::kMap) {                                 \
        for (auto v : args) {                                              \
            std::cerr << v.ToString() << std::endl;                        \
        }                                                                  \
        EXCEPTION(std::string(__FUNCTION__) + check_error(i, "map", ctx)); \
    }

#define CHECK_PARAMETER_ARRAY(i)                                             \
    if (args[i].Type != ValueType::kArray) {                                 \
        for (auto v : args) {                                                \
            std::cerr << v.ToString() << std::endl;                          \
        }                                                                    \
        EXCEPTION(std::string(__FUNCTION__) + check_error(i, "array", ctx)); \
    }
#endif

#ifndef COUNT_OF
#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#endif