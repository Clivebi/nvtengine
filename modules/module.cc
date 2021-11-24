#include "./openvas/api.cc"
#include "bytes.cc"
#include "http.cc"
#include "json.cc"
#include "tcp.cc"
void RegisgerModulesBuiltinMethod(Executor* vm) {
    RegisgerBytesBuiltinMethod(vm);
    RegisgerTcpBuiltinMethod(vm);
    RegisgerHttpBuiltinMethod(vm);
    RegisgerJsonBuiltinMethod(vm);
    RegisgerOVABuiltinMethod(vm);
}

#include "json/json_parser.cc"
#include "thirdpart/http-parser/http_parser.c"
#include "network/tcpstream.cc"