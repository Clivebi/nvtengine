#include "check.hpp"
using namespace Interpreter;

Value ParseJSON(std::string& str,bool unescape);

Value JSONDecode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    return ParseJSON(args[0].bytes,true);
}

Value JSONEncode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    return args[0].ToJSONString(true);
}

BuiltinMethod jsonMethod[] = {{"JSONDecode", JSONDecode}, {"JSONEncode", JSONEncode}};

void RegisgerJsonBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(jsonMethod, COUNT_OF(jsonMethod));
}