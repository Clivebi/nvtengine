#include <algorithm>
#include <regex>

#include "../engine/check.hpp"
using namespace Interpreter;

Value ContainsBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    if (args[0].IsNULL()) {
        return Value();
    }
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    return Value(p0.find(p1) != std::string::npos);
}

Value HasPrefixBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    return Value(p0.find(p1) == 0);
}

Value HasSuffixBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
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

Value TrimLeftBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
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
    if (head_remove == p0.size()) {
        if (args[0].IsString()) {
            return "";
        }
        return Value::MakeBytes("");
    } else {
        if (args[0].IsString()) {
            return p0.substr(head_remove);
        }
        return Value::MakeBytes(p0.substr(head_remove));
    }
}

Value TrimRightBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
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
    if (args[0].IsString()) {
        return p0.substr(0, p0.size() - remove_size);
    }
    Value ret = Value::MakeBytes(p0.substr(0, p0.size() - remove_size));
    return ret;
}

Value TrimBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
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
    if (head_remove == p0.size()) {
        if (args[0].IsString()) {
            return "";
        }
        return Value::MakeBytes("");
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
    if (args[0].IsString()) {
        return p0.substr(0, p0.size() - remove_size);
    }
    Value ret = Value::MakeBytes(p0.substr(0, p0.size() - remove_size));
    return ret;
}

Value IndexBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = p0.find(p1);
    return Value((long)pos);
}

Value LastIndexBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    size_t pos = p0.rfind(p1);
    return Value((long)pos);
}

Value RepeatBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    int count = GetInt(args, 1);
    std::string p0 = GetString(args, 0);
    std::string result = "";
    for (size_t i = 0; i < count; i++) {
        result.append(p0);
    }
    if (args[0].IsString()) {
        return result;
    }
    return Value::MakeBytes(result);
}

Value ReplaceBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    CHECK_PARAMETER_STRING_OR_BYTES(2);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::string p2 = GetString(args, 2);
    int count = GetInt(args, 3, -1);
    std::string result = p0;
    result = replace_str(result, p1, p2, count);
    if (args[0].IsString()) {
        return result;
    }
    return Value::MakeBytes(result);
}

Value ReplaceAllBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::string p2 = GetString(args, 2);
    std::string result = p0;
    result = replace_str(result, p1, p2, -1);
    if (args[0].IsString()) {
        return result;
    }
    return Value::MakeBytes(result);
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

Value SplitBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string p0 = GetString(args, 0);
    std::string p1 = GetString(args, 1);
    std::list<std::string> result = Split(p0, p1);
    Value ret = Value::MakeArray();
    for (auto iter : result) {
        ret._array().push_back(iter);
    }
    return ret;
}

Value ToUpperBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string p0 = GetString(args, 0);
    transform(p0.begin(), p0.end(), p0.begin(), toupper);
    if (args[0].IsString()) {
        return p0;
    }
    return Value::MakeBytes(p0);
}

Value ToLowerBytesOrString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string p0 = GetString(args, 0);
    transform(p0.begin(), p0.end(), p0.begin(), tolower);
    if (args[0].IsString()) {
        return p0;
    }
    return Value::MakeBytes(p0);
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
    if (args[0].IsNULL()) {
        return Value();
    }
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
    Value ret = Value::MakeArray();
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
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string src = GetString(args, 0);
    size_t n = src.size();
    size_t i = 0;
    std::string result = "";
    while (i < n) {
        std::string hex = "";
        std::string str = "";
        char buffer[6] = {0};
        for (int j = 0; j < 16; j++, i++) {
            if (i < n) {
                sprintf(buffer, "%02X ", (BYTE)src[i]);
                hex += buffer;
                if (isprint(src[i])) {
                    str += (char)src[i];
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

Value CopyBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string src = GetString(args, 1);
    args[0].bytesView.CopyFrom(src.c_str(),src.size());
    return args[0];
}

BuiltinMethod bytesMethod[] = {{"ContainsBytes", ContainsBytesOrString},
                               {"HasPrefixBytes", HasPrefixBytesOrString},
                               {"HasSuffixBytes", HasSuffixBytesOrString},
                               {"TrimLeftBytes", TrimLeftBytesOrString},
                               {"TrimRightBytes", TrimRightBytesOrString},
                               {"TrimBytes", TrimBytesOrString},
                               {"IndexBytes", IndexBytesOrString},
                               {"LastIndexBytes", LastIndexBytesOrString},
                               {"RepeatBytes", RepeatBytesOrString},
                               {"ReplaceBytes", ReplaceBytesOrString},
                               {"ReplaceAllBytes", ReplaceAllBytesOrString},
                               {"SplitBytes", SplitBytesOrString},
                               {"ToLowerBytes", ToLowerBytesOrString},
                               {"ToUpperBytes", ToUpperBytesOrString},
                               {"ContainsString", ContainsBytesOrString},
                               {"HasPrefixString", HasPrefixBytesOrString},
                               {"HasSuffixString", HasSuffixBytesOrString},
                               {"TrimLeftString", TrimLeftBytesOrString},
                               {"TrimRightString", TrimRightBytesOrString},
                               {"TrimString", TrimBytesOrString},
                               {"IndexString", IndexBytesOrString},
                               {"LastIndexString", LastIndexBytesOrString},
                               {"RepeatString", RepeatBytesOrString},
                               {"ReplaceString", ReplaceBytesOrString},
                               {"ReplaceAllString", ReplaceAllBytesOrString},
                               {"SplitString", SplitBytesOrString},
                               {"ToLowerString", ToLowerBytesOrString},
                               {"ToUpperString", ToUpperBytesOrString},
                               {"IsMatchRegexp", IsMatchRegexp},
                               {"SearchRegExp", SearchRegExp},
                               {"RegExpReplace", RegExpReplace},
                               {"HexDumpBytes", HexDumpBytes},
                               {"HexDumpString", HexDumpBytes},
                               {"CopyBytes", CopyBytes}};

void RegisgerBytesBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(bytesMethod, COUNT_OF(bytesMethod));
}