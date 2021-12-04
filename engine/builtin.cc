#include <iostream>

#include "vm.hpp"

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

using namespace Interpreter;

#define CHECK_PARAMETER_COUNT(args, count)                              \
    if (args.size() < count) {                                          \
        DEBUG_CONTEXT();                                                \
        throw RuntimeException(std::string(__FUNCTION__) +              \
                               ": the count of parameters not enough"); \
    }
Value Println(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    std::string result;
    for (std::vector<Value>::iterator iter = values.begin(); iter != values.end(); iter++) {
        result += iter->ToString();
        result += " ";
    }
    std::cout << result << std::endl;
    return Value();
}

Value len(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    return Value(arg.Length());
}

Value TypeOf(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    return Value(arg.TypeName());
}

Value ToString(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    return Value(arg.ToString());
}

Value ToByte(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    if (!arg.IsNumber()) {
        DEBUG_CONTEXT();
        throw RuntimeException("only integer can convert to byte");
    }
    return Value((BYTE)arg.Integer);
}

bool IsByteArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (!iter->IsNumber()) {
            return false;
        }
        if (iter->Integer > 255 || iter->Integer < 0) {
            return false;
        }
        iter++;
    }
    return true;
}

bool IsStringArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (!iter->IsStringOrBytes()) {
            return false;
        }
        iter++;
    }
    return true;
}

void AppendIntegerArrayToBytes(Value& val, const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        val.bytes.append(1, (unsigned char)iter->Integer);
        iter++;
    }
}

Value append(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value to = values.front();
    if (to.Type == ValueType::kArray) {
        std::vector<Value>::iterator iter = values.begin();
        iter++;
        while (iter != values.end()) {
            to._array().push_back(*iter);
            iter++;
        }
        return to;
    }
    if (to.Type == ValueType::kBytes) {
        std::vector<Value>::iterator iter = values.begin();
        iter++;
        while (iter != values.end()) {
            switch (iter->Type) {
            case ValueType::kBytes:
            case ValueType::kString:
                to.bytes += iter->bytes;
                break;
            case ValueType::kInteger:
            case ValueType::kByte:
                to.bytes.append(1, (unsigned char)iter->Integer);
                break;
            case ValueType::kArray:
                if (!IsByteArray(iter->_array())) {
                    DEBUG_CONTEXT();
                    throw RuntimeException("only integer array (0~255) can append to bytes");
                }
                AppendIntegerArrayToBytes(to, iter->_array());
                break;
            default:
                DEBUG_CONTEXT();
                throw RuntimeException(iter->ToString() + " can't append to bytes");
            }
            iter++;
        }
        return to;
    }
    DEBUG_CONTEXT();
    throw RuntimeException("first append value must an array or bytes");
}

Value close(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value res = values.front();
    if (res.Type != ValueType::kResource) {
        throw RuntimeException("close parameter must a resource");
    }
    res.resource->Close();
    return Value();
}

Value Exit(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value exitCode = values.front();
    if (!exitCode.IsInteger()) {
        throw RuntimeException("exit parameter must a integer");
    }
    ctx->ExitExecuted(exitCode);
    return Value();
}

Value Require(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    if (!ctx->IsTop()) {
        throw RuntimeException("require must called in top context");
    }
    Value name = values.front();
    if (name.Type != ValueType::kString) {
        throw RuntimeException("require parameter must a string");
    }
    vm->RequireScript(name.bytes, ctx);
    return Value();
}

Value MakeBytes(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    if (values.size() == 0) {
        return Value::make_bytes("");
    }
    Value ret = Value::make_bytes("");

    for (auto arg : values) {
        if (arg.IsStringOrBytes()) {
            ret.bytes += arg.bytes;
            continue;
        }
        if (arg.Type == ValueType::kInteger) {
            ret.bytes += (char)arg.Integer;
            continue;
        }
        if (arg.Type == ValueType::kByte) {
            ret.bytes += (char)arg.Byte;
            continue;
        }
        if (arg.Type == ValueType::kArray) {
            if (!IsByteArray(arg._array())) {
                DEBUG_CONTEXT();
                throw RuntimeException("convert to bytes must use integer array(0~255");
            }
            AppendIntegerArrayToBytes(ret, arg._array());
            continue;
        }
    }
    return ret;
}

Value MakeString(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    if (values.size() == 0) {
        return Value("");
    }
    Value ret = Value::make_bytes("");
    ret.Type = ValueType::kString;

    for (auto arg : values) {
        if (arg.IsStringOrBytes()) {
            ret.bytes += arg.bytes;
            continue;
        }
        if (arg.Type == ValueType::kInteger) {
            ret.bytes += arg.ToString();
            continue;
        }
        if (arg.Type == ValueType::kByte) {
            ret.bytes += (char)arg.Byte;
            continue;
        }
        if (arg.Type == ValueType::kArray) {
            if (!IsByteArray(arg._array())) {
                DEBUG_CONTEXT();
                throw RuntimeException("convert to string must use integer array (0~255)");
            }
            AppendIntegerArrayToBytes(ret, arg._array());
            continue;
        }
    }
    return ret;
}

bool IsHexChar(char c) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        return true;
    }
    return false;
}

Value HexDecodeString(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    if (arg.Type != ValueType::kString) {
        DEBUG_CONTEXT();
        throw RuntimeException("HexDecodeString parameter must a string");
    }
    if (arg.Length() % 2 || arg.Length() == 0) {
        DEBUG_CONTEXT();
        throw RuntimeException("HexDecodeString string length must be a multiple of 2");
    }
    size_t i = 0;
    char buf[3] = {0};
    std::string result = "";
    for (; i < arg.bytes.size(); i += 2) {
        buf[0] = arg.bytes[i];
        buf[1] = arg.bytes[i + 1];
        if (!IsHexChar(buf[0]) || !IsHexChar(buf[1])) {
            DEBUG_CONTEXT();
            throw RuntimeException("HexDecodeString parameter string is not a valid hex string");
        }
        unsigned char val = strtol(buf, NULL, 16);
        result.append(1, val);
    }
    return Value::make_bytes(result);
}

Value HexEncode(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    char buf[16] = {0};
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    switch (arg.Type) {
    case ValueType::kBytes:
    case ValueType::kString:
        return Value(HexEncode(arg.bytes.c_str(), arg.bytes.size()));
    case ValueType::kInteger:
        snprintf(buf, 16, "%llX", arg.Integer);
        return Value(buf);
    default:
        break;
    }
    throw RuntimeException("HexEncode unsupported parameter type<" + arg.TypeName() + ">");
}

Value ToInteger(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    switch (arg.Type) {
    case ValueType::kInteger:
    case ValueType::kByte:
        return arg;
    case ValueType::kFloat:
        return Value((Value::INTVAR)arg.Float);
    case ValueType::kString:
    case ValueType::kBytes: {
        Value::INTVAR val = strtoll(arg.bytes.c_str(), NULL, 0);
        return Value(val);
    }
    default:
        return Value((__LONG_LONG_MAX__));
    }
}

Value ToFloat(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    switch (arg.Type) {
    case ValueType::kInteger:
    case ValueType::kByte:
        return Value((double)arg.Integer);
    case ValueType::kFloat:
        return arg;
    case ValueType::kString:
    case ValueType::kBytes: {
        double val = strtod(arg.bytes.c_str(), NULL);
        return Value(val);
    }
    default:
        return Value((double)0.0);
    }
}

bool IsBigEndianVM() {
    unsigned int value = 0xAA;
    unsigned char* ptr = (unsigned char*)&value;
    if (*ptr == 0xAA) {
        return false;
    }
    return true;
}

Value VMEnv(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    Value ret = Value::make_map();
    if (IsBigEndianVM()) {
        ret._map()[Value("ByteOrder")] = "BigEndian";
    } else {
        ret._map()[Value("ByteOrder")] = "LittleEndian";
    }
    ret._map()["Size Of integer"] = Value(sizeof(Value::INTVAR));
    ret._map()["Engine Version"] = Value(VERSION);
#ifdef __APPLE__
    ret._map()["OS"] = "Darwin";
#endif
#ifdef __WIN32__
    ret._map()["OS"] = "Windows";
#endif
#ifdef __LINUX__
    ret._map()["OS"] = "Linux";
#endif
    return ret;
}

Value GetAvaliableFunction(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    return vm->GetAvailableFunction(ctx);
}

Value IsFunctionExist(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    if (arg.Type != ValueType::kString && arg.Type != ValueType::kBytes) {
        throw RuntimeException("IsFunctionExist parameter must a string");
    }
    Value ret = vm->GetFunction(arg.bytes, ctx);
    if (ret.IsFunction()) {
        return Value(true);
    }
    return Value(false);
}

Value Error(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    Value& arg = values.front();
    if (arg.Type != ValueType::kString && arg.Type != ValueType::kBytes) {
        throw RuntimeException("error parameter must a string");
    }
    throw RuntimeException(arg.bytes);
}

Value DisplayContext(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    if (values.size()) {
        std::cout << ctx->DumpContext(values.front().ToBoolean()) << std::endl;
    } else {
        std::cout << ctx->DumpContext(false) << std::endl;
    }
    return Value();
}

Value Clone(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(values, 1);
    return values.front().Clone();
}

BuiltinMethod builtinFunction[] = {
        {"byte", ToByte},
        {"exit", Exit},
        {"len", len},
        {"clone", Clone},
        {"append", append},
        {"require", Require},
        {"bytes", MakeBytes},
        {"close", close},
        {"typeof", TypeOf},
        {"error", Error},
        {"Println", Println},
        {"ToString", ToString},
        {"ToInteger", ToInteger},
        {"ToFloat", ToFloat},
        {"HexDecodeString", HexDecodeString},
        {"HexEncode", HexEncode},
        {"DisplayContext", DisplayContext},
        {"VMEnv", VMEnv},
        {"GetAvaliableFunction", GetAvaliableFunction},
        {"IsFunctionExist", IsFunctionExist},
};

bool IsFunctionOverwriteEnabled(const std::string& name) {
    for (int i = 0; i < COUNT_OF(builtinFunction); i++) {
        if (builtinFunction[i].name == name) {
            return false;
        }
    }
    return true;
}

void RegisgerEngineBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(builtinFunction, COUNT_OF(builtinFunction));
}