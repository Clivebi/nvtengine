#include "./openvas/api.cc"
#include "base64.cc"
#include "bytes.cc"
#include "http.cc"
#include "json.cc"
#include "netcon.cc"
void RegisgerModulesBuiltinMethod(Executor* vm) {
    RegisgerBytesBuiltinMethod(vm);
    RegisgerNetConnBuiltinMethod(vm);
    RegisgerHttpBuiltinMethod(vm);
    RegisgerJsonBuiltinMethod(vm);
    RegisgerBase64BuiltinMethod(vm);
    RegisgerOVABuiltinMethod(vm);
}

#include "json/json_parser.cc"
#include "net/dial.cc"
#include "net/sslhelper.cc"
#include "thirdpart/http-parser/http_parser.c"