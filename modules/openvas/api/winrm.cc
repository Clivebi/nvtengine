
#ifdef _WIN32
#include "./winrm_win.cc"
#else
#include "../../../winrm/winrm.h"
#include "../api.hpp"
class WINRMHandle : public Resource {
public:
    GoUintptr mHandle;

public:
    explicit WINRMHandle(GoUintptr handle) : mHandle(handle) {}
    ~WINRMHandle() { Close(); }
    void Close() {
        if (mHandle != 0) {
            WinRMClose(mHandle);
            mHandle = 0;
        }
    };
    bool IsAvaliable() { return mHandle != 0; }
    std::string TypeName() { return "WINRM"; };
};

Value CreateWinRM(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(11);
    CHECK_PARAMETER_STRING(0);  //host
    CHECK_PARAMETER_INTEGER(1); //port
    CHECK_PARAMETER_STRING(2);  //login
    CHECK_PARAMETER_STRING(3);  //key
    CHECK_PARAMETER_INTEGER(4); //is_https
    CHECK_PARAMETER_INTEGER(5); //inscure
    CHECK_PARAMETER_STRING(6);  //ca base64
    CHECK_PARAMETER_STRING(7);  //cert base64
    CHECK_PARAMETER_STRING(8);  //cert key
    CHECK_PARAMETER_INTEGER(9); //timeout
    CHECK_PARAMETER_INTEGER(10) //useNTLM
    GoUintptr Handle =
            WinRMOpen((char*)args[0].text.c_str(), args[1].Integer, (char*)args[2].text.c_str(),
                      (char*)args[3].text.c_str(), args[4].Integer, args[5].Integer,
                      (char*)args[6].text.c_str(), (char*)args[7].text.c_str(),
                      (char*)args[8].text.c_str(), args[9].Integer, args[10].Integer);
    if (Handle == 0) {
        return Value();
    }
    return Value((Resource*)new WINRMHandle(Handle));
}

Value WinRMCommand(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    CHECK_PARAMETER_INTEGER(3);
    std::string resname;
    if (!args[0].IsResource(resname) || resname != "WINRM") {
        return Value();
    }
    std::string out;
    std::string errOut;
    WINRMHandle* Handle = (WINRMHandle*)args[0].resource.get();
    WinRMExecute_return result = WinRMExecute(Handle->mHandle, (char*)args[1].text.c_str(),
                                              (char*)args[2].text.c_str(), args[3].Integer);
    Value ret = Value::MakeMap();
    if (result.r4 != NULL) {
        NVT_LOG_WARNING("WinRMExecute have error :", result.r4);
        free(result.r4);
        return Value();
    } else {
        out.assign(result.r0, result.r1);
        errOut = result.r2;
        free(result.r0);
        free(result.r2);
    }
    ret["Stdout"] = out;
    ret["StdErr"] = errOut;
    ret["ExitCode"] = result.r3;
    return ret;
}
#endif