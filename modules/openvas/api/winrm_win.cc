#include <Windows.h>
#define WSMAN_API_VERSION_1_1 1
#include <wsman.h>

#include <string>
#include "../api.hpp"

::std::string base64_encode(const ::std::string& bindata);



std::wstring  A2W(std::string src, UINT codepage = CP_UTF8) {
    std::wstring return_value(L"");
    int len = MultiByteToWideChar(codepage, 0, src.c_str(), src.size(), NULL, 0);
    if (len <= 0) {
        return return_value;
    }
    WCHAR* buffer = new WCHAR[len + 1];
    MultiByteToWideChar(codepage, 0, src.c_str(), src.size(), buffer, len);
    buffer[len] = '\0';
    return_value.append(buffer);
    delete[] buffer;
    return return_value;
}

std::string W2A(std::wstring src, UINT codepage = CP_ACP) {
    std::string return_value("");
    int len = WideCharToMultiByte(codepage, 0, src.c_str(), src.size(), NULL, 0, NULL, NULL);
    if (len <= 0) {
        return return_value;
    }
    char* buffer = new char[len + 1];
    WideCharToMultiByte(codepage, 0, src.c_str(), src.size(), buffer, len, NULL, NULL);
    buffer[len] = '\0';

    return_value.append(buffer);
    delete[] buffer;
    return return_value;
}


class WINRMHandle : public Resource {
public:
    WSMAN_API_HANDLE mHandle;
    WSMAN_SESSION_HANDLE mSession;

public:
    explicit WINRMHandle() : mSession(NULL) {
        if (0 != WSManInitialize(WSMAN_FLAG_REQUESTED_API_VERSION_1_0, &mHandle)) {
            throw RuntimeException("WSManInitialize failed");
        }
    }

    DWORD CreateSession(std::string connection, std::string username, std::string password,
                        int timeout = 60) {
        std::wstring wstrUser = A2W(username);
        std::wstring wstrKey = A2W(password);
        std::wstring wstrConn = A2W(connection);
        WSMAN_AUTHENTICATION_CREDENTIALS cred = {0};
        cred.authenticationMechanism = WSMAN_FLAG_AUTH_NEGOTIATE;
        cred.userAccount.username = wstrUser.c_str();
        cred.userAccount.password = wstrKey.c_str();

        DWORD dwError = WSManCreateSession(mHandle, wstrConn.c_str(), 0, &cred, NULL, &mSession);
        if (dwError == 0) {
            WSMAN_DATA data;
            data.type = WSMAN_DATA_TYPE_DWORD;
            data.number = timeout * 1000;
            WSManSetSessionOption(mSession, WSMAN_OPTION_DEFAULT_OPERATION_TIMEOUTMS, &data);
            data.type = WSMAN_DATA_TYPE_TEXT;
            data.text.buffer = L"en-US";
            data.text.bufferLength = wcslen(L"en-US");
            WSManSetSessionOption(mSession, WSMAN_OPTION_LOCALE, &data);
            WSManSetSessionOption(mSession, WSMAN_OPTION_UI_LANGUAGE, &data);
        }
        return dwError;
    }
    virtual void Close() {
        if (mSession != NULL) {
            WSManCloseSession(mSession, 0);
            mSession = NULL;
        }
        if (mHandle != NULL) {
            WSManDeinitialize(mHandle, 0);
            mHandle = NULL;
        }
    };
    virtual bool IsAvaliable() { return mHandle != 0; }
    virtual std::string TypeName() { return "WINRM"; };
};

#define CALL_TYPE_CREATE_SHELL 1
#define CALL_TYPE_RUN_COMMAND 2
#define CALL_TYPE_RECEIVE_OUTPUT 3
#define CALL_TYPE_CLOSE_COMMAND 4
#define CALL_TYPE_CLOSE_SHELL 5

typedef struct _ASYNC_CONTEXT {
    HANDLE hEvent;
    DWORD CallType;
    std::string strStdout;
    std::string strStderr;
    DWORD ExitCode;
    std::wstring Error;
    DWORD ErrorCode;
} ASYNC_CONTEXT, *PCASYNC_CONTEXT;

VOID CALLBACK AsyncCallbackImplement(PVOID operationContext, DWORD flags, WSMAN_ERROR* error,
                                     WSMAN_SHELL_HANDLE shell, WSMAN_COMMAND_HANDLE command,
                                     WSMAN_OPERATION_HANDLE operationHandle,
                                     WSMAN_RESPONSE_DATA* data) {
    PCASYNC_CONTEXT pContext = (PCASYNC_CONTEXT)operationContext;
    switch (pContext->CallType) {
    case CALL_TYPE_RECEIVE_OUTPUT:
        if (error != NULL && error->code != 0) {
            pContext->ErrorCode = error->code;
            pContext->Error = error->errorDetail;
            break;
        }
        if (data == NULL) {
            return;
        }
        if (data->receiveData.streamData.type == WSMAN_DATA_TYPE_BINARY &&
            data->receiveData.streamData.binaryData.dataLength) {
            if (_wcsicmp(data->receiveData.streamId, WSMAN_STREAM_ID_STDERR) == 0) {
                pContext->strStderr.append((char*)data->receiveData.streamData.binaryData.data,
                                           data->receiveData.streamData.binaryData.dataLength);
            }
            if (_wcsicmp(data->receiveData.streamId, WSMAN_STREAM_ID_STDOUT) == 0) {
                pContext->strStdout.append((char*)data->receiveData.streamData.binaryData.data,
                                           data->receiveData.streamData.binaryData.dataLength);
            }
        }
        if (data->receiveData.commandState &&
            0 == _wcsicmp(data->receiveData.commandState, WSMAN_COMMAND_STATE_DONE)) {
            pContext->ExitCode = data->receiveData.exitCode;
            break;
        }
        return;
    default:
        if (error != NULL && error->code != 0) {
            pContext->ErrorCode = error->code;
            pContext->Error = error->errorDetail;
        }
    }
    SetEvent(pContext->hEvent);
}

struct _ExecuteCommandResult {
    std::string Stdout;
    std::string StdError;
    std::wstring Error;
    int ErrorCode;
    int ExitCode;
};

std::string BuildPowerShellCommand(std::string cmd) {
    std::string fullCmd = "$ProgressPreference = 'SilentlyContinue';" + cmd;
    std::wstring strCmd = A2W(fullCmd);
    std::string mem;
    mem.assign((char*)strCmd.c_str(), strCmd.size() * sizeof(wchar_t));
    return "powershell.exe -EncodedCommand " + base64_encode(mem);
}

//Set-Item -Path WSMan:\localhost\Client\TrustedHosts -Value '*'
BOOL ExecuteCommand(WINRMHandle* winRM, std::string cmd, bool usePS,
                    _ExecuteCommandResult& result) {
    WSMAN_SHELL_ASYNC async = {0};
    ASYNC_CONTEXT context;
    WSMAN_SHELL_HANDLE hShell = NULL;
    WSMAN_COMMAND_HANDLE hCommand = NULL;
    WSMAN_OPERATION_HANDLE hOperation = NULL;
    BOOL bResult = FALSE;

    context.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    async.completionFunction = AsyncCallbackImplement;
    async.operationContext = &context;
    context.CallType = CALL_TYPE_CREATE_SHELL;
    context.ErrorCode = 0;
    context.ExitCode = -1;

    if (usePS) {
        cmd = BuildPowerShellCommand(cmd);
    }

    do {
        context.CallType = CALL_TYPE_CREATE_SHELL;
        WSManCreateShell(winRM->mSession, 0, WSMAN_CMDSHELL_URI, NULL, NULL, NULL, &async, &hShell);
        if (ERROR_SUCCESS != WaitForSingleObject(context.hEvent, INFINITE) || context.ErrorCode !=0) {
            break;
        }
        context.CallType = CALL_TYPE_RUN_COMMAND;
        WSManRunShellCommand(hShell, 0, A2W(cmd).c_str(), NULL, NULL, &async, &hCommand);
        if (ERROR_SUCCESS != WaitForSingleObject(context.hEvent, INFINITE) || context.ErrorCode !=0) {
            break;
        }
        context.CallType = CALL_TYPE_RECEIVE_OUTPUT;
        WSManReceiveShellOutput(hShell, hCommand, 0, NULL, &async, &hOperation);
        if (ERROR_SUCCESS != WaitForSingleObject(context.hEvent, INFINITE) || context.ErrorCode != 0) {
            break;
        }
        bResult = TRUE;
    } while (false);
    result.Error = context.Error;
    result.ErrorCode = context.ErrorCode;
    result.ExitCode = context.ExitCode;
    result.StdError = context.strStderr;
    result.Stdout = context.strStdout;

    if (hOperation != NULL) {
        WSManCloseOperation(hOperation, 0);
        hOperation = NULL;
    }
    if (hCommand != NULL) {
        context.CallType = CALL_TYPE_CLOSE_COMMAND;
        WSManCloseCommand(hCommand, 0, &async);
        WaitForSingleObject(context.hEvent, INFINITE);
        hCommand = NULL;
    }
    if (hShell != NULL) {
        context.CallType = CALL_TYPE_CLOSE_SHELL;
        WSManCloseShell(hShell, 0, &async);
        WaitForSingleObject(context.hEvent, INFINITE);
        hShell = NULL;
    }
    CloseHandle(context.hEvent);
    return bResult;
}

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
    std::string connection;
    if (args[4].ToBoolean()) {
        connection = "https://";
    } else {
        connection = "http://";
    }
    connection += GetString(args, 0);
    connection += ":";
    connection += args[1].ToString();
    connection += "/wsman";
    scoped_refptr<WINRMHandle> Handler = new WINRMHandle();
    if (0 == Handler->CreateSession(connection, GetString(args, 2), GetString(args, 3))) {
        return Value((Resource*)Handler.get());
    }
    return Value();
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
    WINRMHandle* Handle = (WINRMHandle*)args[0].resource.get();
    _ExecuteCommandResult result;
    result.ErrorCode = 0;
    result.ExitCode = -1;

    if (!ExecuteCommand(Handle, args[1].text.c_str(), args[3].Integer, result) ||
        result.ErrorCode != 0) {
        LOG_WARNING("WinRMExecute have error :",result.ErrorCode, W2A(result.Error));
        return Value();
    }
    Value ret = Value::MakeMap();
    ret["Stdout"] = result.Stdout;
    ret["StdErr"] = result.StdError;
    ret["ExitCode"] = result.ExitCode;
    return ret;
}