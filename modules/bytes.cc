#include <algorithm>
#include <regex>

#include "check.hpp"
using namespace Interpreter;

Value ContainsBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    return Value(args[0].bytes.find(args[1].bytes) != std::string::npos);
}

Value HasPrefixBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    return Value(args[0].bytes.find(args[1].bytes) == 0);
}

Value HasSuffixBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    if (args[0].bytes.size() < args[1].bytes.size()) {
        return Value(false);
    }
    int pos = args[0].bytes.size() - args[1].bytes.size();
    if (args[0].bytes.substr(pos) == args[1].bytes) {
        return Value(true);
    }
    return Value(false);
}

bool ContainsByte(std::string& str, unsigned char c) {
    return str.find(c) != std::string::npos;
}

Value TrimLeftBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    size_t pos = 0;
    for (size_t i = 0; i < args[0].bytes.size(); i++) {
        if (ContainsByte(args[1].bytes, args[0].bytes[i])) {
            pos = i + 1;
            continue;
        }
        break;
    }
    if (pos < args[0].bytes.size()) {
        Value ret = Value::make_bytes(args[0].bytes.substr(pos));
        ret.Type = args[0].Type;
        return ret;
    }
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    return ret;
}

Value TrimRightBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    if (args[0].bytes.size() == 0) {
        return Value("");
    }
    size_t length = args[0].bytes.size();
    for (size_t i = args[0].bytes.size() - 1; i >= 0; i--) {
        if (ContainsByte(args[1].bytes, args[0].bytes[i])) {
            length = i;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes(args[0].bytes.substr(0, length));
    ret.Type = args[0].Type;
    return ret;
}

Value TrimBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    size_t pos = 0;
    for (size_t i = 0; i < args[0].bytes.size(); i++) {
        if (ContainsByte(args[1].bytes, args[0].bytes[i])) {
            pos = i + 1;
            continue;
        }
        break;
    }
    size_t end = args[0].bytes.size() - 1;
    for (size_t i = args[0].bytes.size() - 1; i >= 0; i--) {
        if (ContainsByte(args[1].bytes, args[0].bytes[i])) {
            end = i;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    if (pos > end) {
        return ret;
    }
    ret.bytes = args[0].bytes.substr(pos, end - pos);
    return ret;
}

Value IndexBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    size_t pos = args[0].bytes.find(args[1].bytes);
    return Value((long)pos);
}

Value LastIndexBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    size_t pos = args[0].bytes.rfind(args[1].bytes);
    return Value((long)pos);
}

Value RepeatBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_INTEGER(1);
    std::string result = "";
    for (size_t i = 0; i < args[1].Integer; i++) {
        result.append(args[0].bytes);
    }
    Value ret = Value::make_bytes(result);
    ret.Type = args[0].Type;
    return ret;
}

std::string& replace_str(std::string& str, const std::string& to_replaced,
                         const std::string& newchars, int maxcount) {
    int count = 0;
    for (std::string::size_type pos(0); pos != std::string::npos; pos += newchars.length()) {
        pos = str.find(to_replaced, pos);
        if (pos != std::string::npos) {
            str.replace(pos, to_replaced.length(), newchars);
            count++;
            if (maxcount != -1 && maxcount == count) {
                break;
            }
        } else {
            break;
        }
    }
    return str;
}

Value ReplaceBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    CHECK_PARAMETER_INTEGER(3);
    std::string result = args[0].bytes;
    result = replace_str(result, args[1].bytes, args[2].bytes, (int)args[3].Integer);
    Value ret = Value::make_bytes(result);
    ret.Type = args[0].Type;
    ret.bytes = result;
    return ret;
}

Value ReplaceAllBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    std::string result = args[0].bytes;
    result = replace_str(result, args[1].bytes, args[2].bytes, -1);
    Value ret = Value::make_bytes(result);
    ret.Type = args[0].Type;
    ret.bytes = result;
    return ret;
}

std::vector<Value> Split(std::string& src, std::string& slip) {
    size_t i = std::string::npos;
    std::string part = src;
    std::vector<Value> result;
    while (true) {
        i = part.find(slip);
        if (i != std::string::npos) {
            result.push_back(part.substr(0, i));
            part = part.substr(i + slip.size());
        } else {
            if (part.size()) {
                result.push_back(part);
            }
            break;
        }
    }
    return result;
}

Value SplitBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    std::vector<Value> result = Split(args[0].bytes, args[1].bytes);
    std::vector<Value>::iterator iter = result.begin();
    while (iter != result.end()) {
        iter->Type = args[1].Type;
        iter++;
    }
    return Value(result);
}

Value ToUpperBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    transform(args[0].bytes.begin(), args[0].bytes.end(), args[0].bytes.begin(), toupper);
    return args[0];
}

Value ToLowerBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    transform(args[0].bytes.begin(), args[0].bytes.end(), args[0].bytes.begin(), tolower);
    return args[0];
}

inline std::regex RegExp(const std::string& r, bool icase) {
    if (icase) {
        return std::regex(r, std::regex_constants::icase);
    }
    return std::regex(r);
}

//buffer,partten,bool icase
Value IsMatchRegexp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    bool icase = true;
    if (args.size() > 2) {
        icase = args[2].ToBoolean();
    }
    std::regex re = RegExp(args[1].bytes, icase);
    bool found = std::regex_search(args[0].bytes.begin(), args[0].bytes.end(), re);
    return Value(found);
}

Value SearchRegExp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    bool icase = true;
    if (args.size() > 2) {
        icase = args[2].ToBoolean();
    }
    std::smatch m;
    std::string s = args[0].bytes;
    Value ret = Value::make_array();
    std::regex re = RegExp(args[1].bytes, icase);
    while (std::regex_search(s, m, re)) {
        for (auto iter = m.begin(); iter != m.end(); iter++) {
            ret._array().push_back(iter->str());
        }
        s = m.suffix().str();
    }
    return ret;
}

Value RegExpReplace(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    bool icase = true;
    if (args.size() > 3) {
        icase = args[3].ToBoolean();
    }
    std::smatch m;
    std::regex re = RegExp(args[1].bytes, icase);
    Value ret = std::regex_replace(args[0].bytes, re, args[2].bytes);
    return ret;
}

BuiltinMethod bytesMethod[] = {{"ContainsBytes", ContainsBytes},
                               {"HasPrefixBytes", HasPrefixBytes},
                               {"HasSuffixBytes", HasSuffixBytes},
                               {"TrimLeftBytes", TrimLeftBytes},
                               {"TrimRightBytes", TrimRightBytes},
                               {"TrimBytes", TrimBytes},
                               {"IndexBytes", IndexBytes},
                               {"LastIndexBytes", LastIndexBytes},
                               {"RepeatBytes", RepeatBytes},
                               {"ReplaceBytes", ReplaceBytes},
                               {"ReplaceAllBytes", ReplaceAllBytes},
                               {"SplitBytes", SplitBytes},
                               {"ToLowerBytes", ToLowerBytes},
                               {"ToUpperBytes", ToUpperBytes},
                               {"ContainsString", ContainsBytes},
                               {"HasPrefixString", HasPrefixBytes},
                               {"HasSuffixString", HasSuffixBytes},
                               {"TrimLeftString", TrimLeftBytes},
                               {"TrimRightString", TrimRightBytes},
                               {"TrimString", TrimBytes},
                               {"IndexString", IndexBytes},
                               {"LastIndexString", LastIndexBytes},
                               {"RepeatString", RepeatBytes},
                               {"ReplaceString", ReplaceBytes},
                               {"ReplaceAllString", ReplaceAllBytes},
                               {"SplitString", SplitBytes},
                               {"ToLowerString", ToLowerBytes},
                               {"ToUpperString", ToUpperBytes},
                               {"IsMatchRegexp", IsMatchRegexp},
                               {"SearchRegExp", SearchRegExp},
                               {"RegExpReplace", RegExpReplace}};

void RegisgerBytesBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(bytesMethod, COUNT_OF(bytesMethod));
}