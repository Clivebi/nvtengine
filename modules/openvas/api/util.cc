#include <unistd.h>

#include <algorithm>
#include <sstream>

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

Value vendor_version(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return Value("NVTEngine 0.1");
}

Value GetHostName(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    char name[260] = {0};
    unsigned int size = 260;
    gethostname(name, size);
    return Value(name);
}

bool ishex(char c);

char http_unhex(char c);

std::string decode_string(const std::string& src) {
    std::stringstream o;
    size_t start = 0;
    for (size_t i = 0; i < src.size();) {
        if (src[i] != '\\') {
            i++;
            continue;
        }
        if (start < i) {
            o << src.substr(start, i - start);
        }
        if (i + 1 >= src.size()) {
            return src;
        }
        i++;
        switch (src[i]) {
        case 'r':
            o << "\r";
            i++;
            break;
        case 'n':
            o << "\n";
            i++;
            break;
        case 't':
            o << "\t";
            i++;
            break;
        case '\"':
            o << "\"";
            i++;
            break;
        case '\\':
            o << "\\";
            i++;
            break;
        default: {
            if (src.size() > i + 1 && ishex(src[i]) && ishex(src[i + 1])) {
                int x = http_unhex(src[i]);
                x *= 16;
                x += http_unhex(src[i + 1]);
                o << (char)(x & 0xFF);
                i += 2;
            } else {
                i++;
                o << src[i];
                LOG("Parse string error :" + src);
            }
        }
        }
        start = i;
    }
    if (start < src.size()) {
        o << src.substr(start);
    }
    return o.str();
}

Value NASLString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    std::string ret = "";
    for (auto iter : args) {
        ret += decode_string(iter.ToString());
    }
    return ret;
}
