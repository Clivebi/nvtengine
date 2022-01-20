#include <iostream>

#include "check.hpp"
#include "vm.hpp"

using namespace Interpreter;

Value Println(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    std::string result;
    std::string name;
    for (std::vector<Value>::iterator iter = values.begin(); iter != values.end(); iter++) {
        result += iter->ToString();
        result += " ";
    }
    std::cout << result << std::endl;
    return Value();
}

Value DumpVar(std::vector<Value>& values, VMContext* ctx, Executor* vm) {
    std::string result;
    std::string name;
    for (std::vector<Value>::iterator iter = values.begin(); iter != values.end(); iter++) {
        result += iter->ToDescription();
        result += " ";
    }
    std::cout << result << std::endl;
    return Value();
}

Value len(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    return Value(arg.Length());
}

Value Delete(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    switch (args[0].Type) {
    case ValueType::kBytes:
    case ValueType::kString:
        break;
    case ValueType::kArray: {
        auto iter = args[0]._array().begin();
        iter += (size_t)args[1].Integer;
        args[0]._array().erase(iter);
    }
    case ValueType::kMap: {
        auto iter = args[0]._map().find(args[1]);
        if (iter != args[0]._map().end()) {
            args[0]._map().erase(iter);
        }
    }
    default:
        break;
    }
    return Value();
}

Value TypeOf(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    return Value(arg.TypeName());
}

Value ToString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    return Value(arg.ToString());
}

Value ToByte(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    if (!arg.IsNumber()) {
        DUMP_CONTEXT();
        throw RuntimeException("only integer can convert to byte");
    }
    return Value((BYTE)arg.Integer);
}

bool IsByteArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (!iter->IsInteger()) {
            return false;
        }
        iter++;
    }
    return true;
}

bool IsStringArray(const std::vector<Value>& values) {
    std::vector<Value>::const_iterator iter = values.begin();
    while (iter != values.end()) {
        if (!iter->IsString()) {
            return false;
        }
        iter++;
    }
    return true;
}

Value append(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value to = args.front();
    if (to.Type == ValueType::kArray) {
        std::vector<Value>::iterator iter = args.begin();
        iter++;
        while (iter != args.end()) {
            to._array().push_back(*iter);
            iter++;
        }
        return to;
    }
    if (to.IsBytes()) {
        std::vector<Value>::iterator iter = args.begin();
        iter++;
        while (iter != args.end()) {
            switch (iter->Type) {
            case ValueType::kBytes:
                to.bytesView = to.bytesView + iter->bytesView;
                break;
            default:
                DUMP_CONTEXT();
                throw RuntimeException(iter->ToDescription() + " can't append to bytes");
            }
            iter++;
        }
        return to;
    }
    DUMP_CONTEXT();
    throw RuntimeException("first append value must an array or bytes");
}

Value close(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    args.front().resource->Close();
    return Value();
}

Value Exit(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value exitCode = args.front();
    if (!exitCode.IsInteger()) {
        throw RuntimeException("exit parameter must a integer");
    }
    ctx->ExitExecuted(exitCode);
    return Value();
}

Value Require(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    if (!ctx->IsTop()) {
        throw RuntimeException("require must called in top context");
    }
    Value name = args.front();
    if (!name.IsString()) {
        throw RuntimeException("require parameter must a string");
    }
    vm->RequireScript(name.text, ctx);
    return Value();
}

Value MakeBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
    return Value::MakeBytes((size_t)args[0].Integer);
}

Value ToBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    if (args.size() == 0) {
        return Value::MakeBytes("");
    }
    CHECK_PARAMETER_COUNT(1);
    if (args[0].IsBytes()) {
        return args[0].Clone();
    }
    CHECK_PARAMETER_STRING(0);
    return Value::MakeBytes(args[0].text);
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

Value HexDecodeString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    if (!arg.IsString()) {
        DUMP_CONTEXT();
        throw RuntimeException("HexDecodeString parameter must a string");
    }
    if (arg.Length() % 2 || arg.Length() == 0) {
        DUMP_CONTEXT();
        throw RuntimeException("HexDecodeString string length must be a multiple of 2");
    }
    size_t i = 0;
    char buf[3] = {0};
    std::string result = "";
    for (; i < arg.text.size(); i += 2) {
        buf[0] = arg.text[i];
        buf[1] = arg.text[i + 1];
        if (!IsHexChar(buf[0]) || !IsHexChar(buf[1])) {
            DUMP_CONTEXT();
            throw RuntimeException("HexDecodeString parameter string is not a valid hex string");
        }
        unsigned char val = (BYTE)strtol(buf, NULL, 16);
        result.append(1, val);
    }
    return Value::MakeBytes(result);
}

Value HexEncode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    char buf[16] = {0};
    CHECK_PARAMETER_COUNT(1);
    Value& arg = args.front();
    switch (arg.Type) {
    case ValueType::kBytes: {
        std::string data = arg.ToString();
        return Value(HexEncode(data.c_str(), data.size()));
    }
    case ValueType::kString:
        return Value(HexEncode(arg.text.c_str(), arg.text.size()));
    case ValueType::kInteger:
        snprintf(buf, 16, "%llX", arg.Integer);
        return Value(buf);
    default:
        break;
    }
    throw RuntimeException("HexEncode unsupported parameter type<" + arg.TypeName() + ">");
}

Value ToInteger(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    return args.front().ToInteger();
}

Value ToFloat(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    return args.front().ToFloat();
}

bool IsBigEndianVM() {
    unsigned int value = 0xAA;
    unsigned char* ptr = (unsigned char*)&value;
    if (*ptr == 0xAA) {
        return false;
    }
    return true;
}

Value VMEnv(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    Value ret = Value::MakeMap();
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

Value GetAvaliableFunction(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return vm->GetAvailableFunction(ctx);
}

Value IsFunctionExist(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    Value ret = vm->GetFunction(args.front().ToString(), ctx);
    if (ret.IsFunction()) {
        return Value(true);
    }
    return Value(false);
}

Value Error(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    throw RuntimeException(args.front().ToString());
}

Value DisplayContext(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    if (args.size()) {
        std::cout << ctx->DumpContext(args.front().ToBoolean()) << std::endl;
    } else {
        std::cout << ctx->DumpContext(false) << std::endl;
    }
    return Value();
}

Value Clone(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    return args.front().Clone();
}

Value ShortStack(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return ctx->ShortStack();
}

BuiltinMethod builtinFunction[] = {
        {"byte", ToByte},
        {"exit", Exit},
        {"len", len},
        {"clone", Clone},
        {"append", append},
        {"delete", Delete},
        {"require", Require},
        {"bytes", ToBytes},
        {"MakeBytes", MakeBytes},
        {"close", close},
        {"typeof", TypeOf},
        {"error", Error},
        {"Println", Println},
        {"string", ToString},
        {"ToString", ToString},
        {"ToInteger", ToInteger},
        {"ToFloat", ToFloat},
        {"DumpVar", DumpVar},
        {"HexDecodeString", HexDecodeString},
        {"HexEncode", HexEncode},
        {"DisplayContext", DisplayContext},
        {"VMEnv", VMEnv},
        {"ShortStack", ShortStack},
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