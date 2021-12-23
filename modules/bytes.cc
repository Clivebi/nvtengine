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
    size_t head_remove = 0;
    for (size_t i = 0; i < p0.size(); i++) {
        if (ContainsByte(p1, p0[i])) {
            head_remove++;
        }
        break;
    }
    if(head_remove == p0.size()){
        return Value::make_bytes("");
    }
    return Value::make_bytes(p0.substr(head_remove));
}

Value TrimRightBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    if (p0.size() == 0) {
        return Value("");
    }
    size_t remove_size = 0;
    for (int i = p0.size() - 1; i > 0; i--) {
        if (ContainsByte(p1, p0[i])) {
            remove_size++;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes(p0.substr(0, p0.size() - remove_size));
    return ret;
}

Value TrimBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t head_remove = 0;
    for (size_t i = 0; i < p0.size(); i++) {
        if (ContainsByte(p1, p0[i])) {
            head_remove++;
            continue;
        }
        break;
    }
    if(head_remove == p0.size()){
        return Value::make_bytes("");
    }
    p0 = p0.substr(head_remove);
    size_t remove_size = 0;
    for (int i = p0.size() - 1; i > 0; i--) {
        if (ContainsByte(p1, p0[i])) {
            remove_size++;
            continue;
        }
        break;
    }
    Value ret = Value::make_bytes(p0.substr(0, p0.size() - remove_size));
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

std::list<std::string> Split(const std::string& src, const std::string& slip) {
    std::list<std::string> result;
    if (src.size() == 0) {
        return result;
    }
    if (slip.size() == 0) {
        result.push_back(src);
        return result;
    }
    size_t pos = 0;
    size_t start = 0;
    while (start < src.size()) {
        pos = src.find(slip, start);
        if (pos == std::string::npos) {
            break;
        }
        result.push_back(src.substr(start, pos - start));
        start = pos + slip.size();
    }
    if (start < src.size()) {
        result.push_back(src.substr(start));
    } else if (start == src.size()) {
        result.push_back("");
    }
    return result;
}

Value SplitBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::list<std::string> result = Split(p0, p1);
    Value ret = Value::make_array();
    for (auto iter : result) {
        ret._array().push_back(iter);
    }
    return ret;
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
    std::string msg = "";
    try {
        if (icase) {
            return std::regex(r, std::regex_constants::icase);
        }
        return std::regex(r);
    } catch (std::regex_error err) {
        msg = err.what();
    }
    throw RuntimeException(r + " is not a valid regex expression " + msg);
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

Value HexDumpBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    size_t n = args[0].Length();
    size_t i = 0;
    std::string result = "";
    while (i < n) {
        std::string hex = "";
        std::string str = "";
        char buffer[6] = {0};
        for (int j = 0; j < 16; j++, i++) {
            if (i < n) {
                sprintf(buffer, "%02X ", (BYTE)args[0].bytes[i]);
                hex += buffer;
                if (isprint(args[0].bytes[i])) {
                    str += (char)args[0].bytes[i];
                } else {
                    str += ".";
                }
            } else {
                hex += "   ";
                str += " ";
            }
        }
        result += hex;
        result += "\t";
        result += str;
        result += "\n";
    }
    return result;
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
                               {"RegExpReplace", RegExpReplace},
                               {"HexDumpBytes", HexDumpBytes},
                               {"HexDumpString", HexDumpBytes}};

void RegisgerBytesBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(bytesMethod, COUNT_OF(bytesMethod));
}