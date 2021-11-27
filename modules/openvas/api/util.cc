#include <unistd.h>

#include <algorithm>

#include "../api.hpp"

Value match(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    bool icase = false;
    if (args.size() > 2 && args[2].ToBoolean()) {
        icase = true;
    }
    if (icase) {
        std::string src = args[0].bytes;
        std::string pattern = args[1].bytes;
        std::transform(src.begin(), src.end(), src.begin(), tolower);
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), tolower);
        return Interpreter::IsMatchString(src, pattern);
    }
    return Interpreter::IsMatchString(args[0].bytes, args[1].bytes);
}

Value Rand(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    srand((int)time(NULL));
    return rand();
}

Value USleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef WIN32
    Sleep(args[0].Integer);
#else
    usleep(args[0].Integer);
#endif
    return Value();
}

Value Sleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef WIN32
    Sleep(args[0].Integer * 1000);
#else
    sleep(args[0].Integer);
#endif
    return Value();
}

Value vendor_version(std::vector<Value>& args, VMContext* ctx, Executor* vm){
    return Value("NVTEngine 0.1");
}

Value GetHostName(std::vector<Value>& args, VMContext* ctx, Executor* vm){
    char name[260] = {0};
    unsigned int size = 260;
    gethostname(name,size);
    return Value(name);
}