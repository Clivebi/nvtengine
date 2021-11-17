#include "bytes.cc"
#include "tcp.cc"
#include "http.cc"
#include "json.cc"
void RegisgerModulesBuiltinMethod(Executor* vm) {
    RegisgerBytesBuiltinMethod(vm);
    RegisgerTcpBuiltinMethod(vm);
    RegisgerHttpBuiltinMethod(vm);
    RegisgerJsonBuiltinMethod(vm);
}


#include "thirdpart/http-parser/http_parser.c"
#include "json/json_parser.cc"