#include <algorithm>
#include <regex>

#include "../engine/check.hpp"
using namespace Interpreter;

Value ContainsBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    if (args[0].IsStringOrBytes() && args[1].IsStringOrBytes()) {
        return Value(args[0].bytes.find(args[1].bytes) != std::string::npos);
    }
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    return Value(p0.find(p1) != std::string::npos);
}

Value HasPrefixBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    if (args[0].IsStringOrBytes() && args[1].IsStringOrBytes()) {
        return Value(args[0].bytes.find(args[1].bytes) == 0);
    }
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    return Value(p0.find(p1) == 0);
}

Value HasSuffixBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    if (p0.size() < p1.size()) {
        return Value(false);
    }
    int pos = p0.size() - p1.size();
    if (p0.substr(pos) == p1) {
        return Value(true);
    }
    return Value(false);
}

bool ContainsByte(std::string& str, unsigned char c) {
    return str.find(c) != std::string::npos;
}

Value TrimLeftBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = 0;
    for (size_t i = 0; i < p0.size(); i++) {
        if (ContainsByte(p1, p0[i])) {
            pos = i + 1;
            continue;
        }
        break;
    }
    if (pos < p0.size()) {
        Value ret = Value::make_bytes(p0.substr(pos));
        return ret;
    }
    Value ret = Value::make_bytes("");
    return ret;
}

Value TrimRightBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    if (p0.size() == 0) {
        return Value("");
    }
    size_t length = p0.size();
    for (size_t i = p0.size() - 1; i >= 0; i--) {
        if (ContainsByte(p1, p0[i])) {
            length = i;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes(p0.substr(0, length));
    return ret;
}

Value TrimBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = 0;
    for (size_t i = 0; i < p0.size(); i++) {
        if (ContainsByte(p1, p0[i])) {
            pos = i + 1;
            continue;
        }
        break;
    }
    size_t end = p0.size() - 1;
    for (size_t i = p0.size() - 1; i >= 0; i--) {
        if (ContainsByte(p1, p0[i])) {
            end = i;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes("");
    if (pos > end) {
        return ret;
    }
    ret.bytes = p0.substr(pos, end - pos);
    return ret;
}

Value IndexBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    if (args[0].IsStringOrBytes() && args[1].IsStringOrBytes()) {
        return Value(args[0].bytes.find(args[1].bytes));
    }
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = p0.find(p1);
    return Value((long)pos);
}

Value LastIndexBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    if (args[0].IsStringOrBytes() && args[1].IsStringOrBytes()) {
        return Value(args[0].bytes.rfind(args[1].bytes));
    }
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = p0.rfind(p1);
    return Value((long)pos);
}

Value RepeatBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    int count = GetInt(args, 1);
    std::string p0 = GetString(args, 0);
    std::string result = "";
    for (size_t i = 0; i < count; i++) {
        result.append(p0);
    }
    Value ret = Value::make_bytes(result);
    return ret;
}

std::string& replace_str(std::string& str, const std::string& to_replaced,
                         const std::string& newchars, int maxcount) {
    int count = 0;
    if (to_replaced.size() == 0 || str.size() == 0) {
        return str;
    }
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
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::string p2 = GetString(args, 2);
    int count = GetInt(args, 3, -1);
    std::string result = p0;
    result = replace_str(result, p1, p2, count);
    Value ret = Value::make_bytes(result);
    ret.bytes = result;
    return ret;
}

Value ReplaceAllBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::string p2 = GetString(args, 2);
    std::string result = p0;
    result = replace_str(result, p1, p2, -1);
    Value ret = Value::make_bytes(result);
    ret.bytes = result;
    return ret;
}

std::vector<Value> Split(std::string& src, std::string& slip) {
    size_t i = std::string::npos;
    std::string part = src;
    std::vector<Value> result;
    if (src.size() == 0 || slip.size() == 0) {
        return result;
    }
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
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::vector<Value> result = Split(p0, p1);
    std::vector<Value>::iterator iter = result.begin();
    while (iter != result.end()) {
        iter++;
    }
    return Value(result);
}

Value ToUpperBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string p0 = GetString(args, 0);
    transform(p0.begin(), p0.end(), p0.begin(), toupper);
    return p0;
}

Value ToLowerBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string p0 = GetString(args, 0);
    transform(p0.begin(), p0.end(), p0.begin(), tolower);
    return p0;
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
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    bool icase = true;
    if (args.size() > 2) {
        icase = args[2].ToBoolean();
    }
    std::regex re = RegExp(p1, icase);
    bool found = std::regex_search(p0.begin(), p0.end(), re);
    return Value(found);
}

Value SearchRegExp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    bool icase = true;
    if (args.size() > 2) {
        icase = args[2].ToBoolean();
    }
    std::smatch m;
    std::string s = p0;
    Value ret = Value::make_array();
    std::regex re = RegExp(p1, icase);
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
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::string p2 = GetString(args, 2);
    bool icase = true;
    if (args.size() > 3) {
        icase = args[3].ToBoolean();
    }
    std::smatch m;
    std::regex re = RegExp(p1, icase);
    Value ret = std::regex_replace(p0, re, p2);
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