#include <iostream>

#include "vm.hpp"

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

using namespace Interpreter;

#define CHECK_PARAMETER_COUNT(args, count)                              \
    if (args.size() < count) {                                          \
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

bool IsIntegerArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (iter->Type != ValueType::kInteger) {
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
                to.bytes += iter->bytes;
                break;
            case ValueType::kInteger:
                to.bytes.append(1, (unsigned char)iter->Integer);
                break;
            case ValueType::kArray:
                if (!IsIntegerArray(iter->_array())) {
                    throw RuntimeException("only integer array can append to bytes");
                }
                AppendIntegerArrayToBytes(to, iter->_array());
                break;
            default:
                throw RuntimeException(iter->ToString() + " can't append to bytes");
            }
            iter++;
        }
        return to;
    }
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
    if (exitCode.Type != ValueType::kInteger) {
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
    if (values.size() == 1) {
        if (values[0].Type == ValueType::kString || values[0].Type == ValueType::kBytes) {
            return Value::make_bytes(values[0].bytes);
        }
        if (values[0].Type == ValueType::kArray) {
            if (!IsIntegerArray(values[0]._array())) {
                throw RuntimeException("make bytes must use integer array");
            }
            Value ret = Value::make_bytes("");
            AppendIntegerArrayToBytes(ret, values[0]._array());
            return ret;
        }
    }
    if (!IsIntegerArray(values)) {
        throw RuntimeException("make bytes must use integer array");
    }
    Value ret = Value::make_bytes("");
    AppendIntegerArrayToBytes(ret, values);
    return ret;
}

Value MakeString(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    if (values.size() == 0) {
        return Value("");
    }
    if (values.size() == 1) {
        if (values[0].Type == ValueType::kString || values[0].Type == ValueType::kBytes) {
            return Value(values[0].bytes);
        }
        if (values[0].Type == ValueType::kFloat || values[0].Type == ValueType::kInteger) {
            return Value(values[0].ToString());
        }
        if (values[0].Type == ValueType::kArray) {
            if (!IsIntegerArray(values[0]._array())) {
                throw RuntimeException("convert to string must use integer array");
            }
            Value ret = Value::make_bytes("");
            AppendIntegerArrayToBytes(ret, values[0]._array());
            ret.Type = ValueType::kString;
            return ret;
        }
    }
    if (!IsIntegerArray(values)) {
        throw RuntimeException("convert to string must use integer array");
    }
    Value ret = Value::make_bytes("");
    AppendIntegerArrayToBytes(ret, values);
    ret.Type = ValueType::kString;
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
        throw RuntimeException("HexDecodeString parameter must a string");
    }
    if (arg.Length() % 2 || arg.Length() == 0) {
        throw RuntimeException("HexDecodeString string length must be a multiple of 2");
    }
    size_t i = 0;
    char buf[3] = {0};
    std::string result = "";
    for (; i < arg.bytes.size(); i += 2) {
        buf[0] = arg.bytes[i];
        buf[1] = arg.bytes[i + 1];
        if (!IsHexChar(buf[0]) || !IsHexChar(buf[1])) {
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

BuiltinMethod builtinFunction[] = {{"exit", Exit},
                                   {"len", len},
                                   {"append", append},
                                   {"require", Require},
                                   {"bytes", MakeBytes},
                                   {"string", MakeString},
                                   {"close", close},
                                   {"typeof", TypeOf},
                                   {"Println", Println},
                                   {"ToString", ToString},
                                   {"ToInteger", ToInteger},
                                   {"ToFloat", ToFloat},
                                   {"HexDecodeString", HexDecodeString},
                                   {"HexEncode", HexEncode},
                                   {"VMEnv", VMEnv},
                                   {"GetAvaliableFunction", GetAvaliableFunction}};

bool IsFunctionOverwriteEnabled(const std::string& name) {
    for (int i = 0; i < 8; i++) {
        if (builtinFunction[i].name == name) {
            return false;
        }
    }
    return true;
}

void RegisgerEngineBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(builtinFunction, COUNT_OF(builtinFunction));
}