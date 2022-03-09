#include "../api.hpp"
#define SNMP_VERSION_1 0
#define SNMP_VERSION_2c 1
#define SNMP_VERSION_2u 2 /* not (will never be) supported by this code */
#define SNMP_VERSION_3 3
int snmp_get(struct snmp_session* session, const char* oid_str, char** result);
int snmpv3_get(const char* peername, const char* username, const char* authpass, int authproto,
               const char* privpass, int privproto, const char* oid_str, char** result);
int snmpv1v2c_get(const char* peername, const char* community, const char* oid_str, int version,
                  char** result);
//peername,community,oid
Value SNMPV1Get(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    CHECK_PARAMETER_STRING_OR_BYTES(2);
    std::string peer = GetString(args, 0);
    std::string community = GetString(args, 1);
    std::string oid = GetString(args, 2);
    std::string result = "";
    char* result_ptr = NULL;
    int error = snmpv1v2c_get(peer.c_str(), community.c_str(), oid.c_str(), SNMP_VERSION_1,
                              &result_ptr);
    Value ret = Value::MakeArray();
    if (result_ptr != NULL) {
        result = result_ptr;
        free(result_ptr);
    }
    ret._array().push_back(error);
    ret._array().push_back(result);
    return ret;
}

//peername,community,oid
Value SNMPV2Get(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    CHECK_PARAMETER_STRING_OR_BYTES(2);
    std::string peer = GetString(args, 0);
    std::string community = GetString(args, 1);
    std::string oid = GetString(args, 2);
    std::string result = "";
    char* result_ptr = NULL;
    int error = snmpv1v2c_get(peer.c_str(), community.c_str(), oid.c_str(), SNMP_VERSION_2c,
                              &result_ptr);
    Value ret = Value::MakeArray();
    if (result_ptr != NULL) {
        result = result_ptr;
        free(result_ptr);
    }
    ret._array().push_back(error);
    ret._array().push_back(result);
    return ret;
}
//peer,username,authpass,authproto,privpass,privproto,oid
Value SNMPV3Get(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(7);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    CHECK_PARAMETER_STRING_OR_BYTES(2);
    CHECK_PARAMETER_STRING_OR_BYTES(3);
    CHECK_PARAMETER_STRING_OR_BYTES(4);
    CHECK_PARAMETER_STRING_OR_BYTES(5);
    CHECK_PARAMETER_STRING_OR_BYTES(6);
    std::string peer = GetString(args, 0);
    std::string username = GetString(args, 1);
    std::string authpass = GetString(args, 2);
    std::string authproto = GetString(args, 3);
    std::string privpass = GetString(args, 4);
    std::string privproto = GetString(args, 5);
    std::string oid = GetString(args, 6);
    std::string result = "";
    char* result_ptr = NULL;
    int authMethod = 0, privMethod = 0;
    if (authproto == "md5") {
        authMethod = 0;
    } else if (authproto == "sha1") {
        authMethod = 1;
    } else {
        throw RuntimeException("invalid authproto type:" + authproto);
    }
    if (privproto == "des") {
        privMethod = 0;
    } else if (privproto == "aes") {
        privMethod = 1;
    } else {
        throw RuntimeException("invalid privproto type:" + privproto);
    }
    int error = snmpv3_get(peer.c_str(), username.c_str(), authpass.c_str(), authMethod,
                           privpass.c_str(), privMethod, oid.c_str(), &result_ptr);
    Value ret = Value::MakeArray();
    if (result_ptr != NULL) {
        result = result_ptr;
        free(result_ptr);
    }
    ret._array().push_back(error);
    ret._array().push_back(result);
    return ret;
}
