#include "bytes.cc"
#include "http.cc"
#include "json.cc"
#include "netcon.cc"
#include "base64.cc"
#include "./openvas/api.cc"
void RegisgerModulesBuiltinMethod(Executor* vm) {
    RegisgerBytesBuiltinMethod(vm);
    RegisgerNetConnBuiltinMethod(vm);
    RegisgerHttpBuiltinMethod(vm);
    RegisgerJsonBuiltinMethod(vm);
    RegisgerOVABuiltinMethod(vm);
}

#include "json/json_parser.cc"
#include "net/dial.cc"
#include "net/sslhelper.cc"
#include "thirdpart/http-parser/http_parser.c"