#pragma once
#include "engine/vm.hpp"

using namespace Interpreter;

namespace rpc {

class JsonRpcHandler {
public:
    virtual Interpreter::Value ProcessRequest(Interpreter::Value& req) = 0;
};
class BaseJsonRpcHandler : public JsonRpcHandler {
public:
    BaseJsonRpcHandler() {}
    virtual ~BaseJsonRpcHandler() {}

protected:
    //-->{"jsonrpc": "2.0", "method": "update", "params": [1,2,3,4,5]}
    //<--{"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": "1"}
    //<--{"jsonrpc": "2.0", "result": 19, "id": 1}
    Interpreter::Value BuildResponse(int code, const char* error, Interpreter::Value id,
                                     Interpreter::Value result) {
        Interpreter::Value response = Interpreter::Value::MakeMap();
        response["jsonrpc"] = "2.0";
        if (!id.IsNULL()) {
            response["id"] = id;
        }
        if (code != 0 || error != NULL) {
            Interpreter::Value errValue = Value::MakeMap();
            if (code == 0) {
                code = -32603;
            }
            errValue["code"] = code;
            errValue["message"] = error;
            response["error"] = errValue;
            return response;
        }
        response["result"] = result;
        return response;
    }

    Interpreter::Value ProcessRequest(Interpreter::Value& req) {
        if (req.IsMap()) {
            Interpreter::Value version = req.GetValue("jsonrpc");
            Interpreter::Value method = req.GetValue("method");
            Interpreter::Value params = req.GetValue("params");
            Interpreter::Value id = req.GetValue("id");
            if (version.ToString() != "2.0" || !method.IsString()) {
                return BuildResponse(-32600, "Invalid request", id, Value());
            }
            return DispatchRequest(method.ToString(), params, id);
        }
        throw Interpreter::RuntimeException("batch request not support");
    }

    virtual Interpreter::Value DispatchRequest(std::string method, Interpreter::Value& params,
                                               Interpreter::Value& id) {
        return Value();
    }

    /*
    Interpreter::Value DispatchRequest(std::string method, Interpreter::Value& params,
                                         Interpreter::Value& id) {
        bool foundMethod = false;
        if (method == "TestAP2") {
            foundMethod = true;
            if (params.IsNULL() || (params.IsArray() && params.Length() == 0)) {
                return BuildResponse(0, NULL, id, TestAP2());
            }
        }
        if (method == "TestAPI") {
            foundMethod = true;
            if (params.IsArray() && params.Length() == 1) {
                return BuildResponse(0, NULL, id, TestAPI(params[0]));
            }
        }
        if (!foundMethod) {
            return BuildResponse(-32601, "Method not found", id, Value());
        }
        return BuildResponse(-32602, "Invalid params", id, Value());
    }*/

#define BEGIN_REQUEST_MAP()                                                            \
    Interpreter::Value DispatchRequest(std::string method, Interpreter::Value& params, \
                                       Interpreter::Value& id) {                       \
        bool foundMethod = false;

#define END_REQUEST_MAP()                                              \
    if (!foundMethod) {                                                \
        return BuildResponse(-32601, "Method not found", id, Value()); \
    }                                                                  \
    return BuildResponse(-32602, "Invalid params", id, Value());       \
    }

#define ADD_HANDLE(handle)                                                   \
    if (method == #handle) {                                                 \
        foundMethod = true;                                                  \
        if (params.IsNULL() || (params.IsArray() && params.Length() == 0)) { \
            return BuildResponse(0, NULL, id, handle());                     \
        }                                                                    \
    }

#define ADD_HANDLE1(handle)                                       \
    if (method == #handle) {                                      \
        foundMethod = true;                                       \
        if (params.IsArray() && params.Length() == 1) {           \
            return BuildResponse(0, NULL, id, handle(params[0])); \
        }                                                         \
    }

#define ADD_HANDLE2(handle)                                                  \
    if (method == #handle) {                                                 \
        foundMethod = true;                                                  \
        if (params.IsArray() && params.Length() == 2) {                      \
            return BuildResponse(0, NULL, id, handle(params[0], params[1])); \
        }                                                                    \
    }

#define ADD_HANDLE3(handle)                                                             \
    if (method == #handle) {                                                            \
        foundMethod = true;                                                             \
        if (params.IsArray() && params.Length() == 3) {                                 \
            return BuildResponse(0, NULL, id, handle(params[0], params[1], params[2])); \
        }                                                                               \
    }

#define ADD_HANDLE4(handle)                                                                        \
    if (method == #handle) {                                                                       \
        foundMethod = true;                                                                        \
        if (params.IsArray() && params.Length() == 4) {                                            \
            return BuildResponse(0, NULL, id, handle(params[0], params[1], params[2], params[2])); \
        }                                                                                          \
    }
};

class SayServer : public BaseJsonRpcHandler {
protected:
    Value SayHello() {
        std::cout << "Hello" << std::endl;
        return Value();
    }
    Value SaySomething(Value& some) {
        std::cout << some.ToString() << std::endl;
        return some;
    }
    Value SayMore(Value& some, Value& more, Value& moremore) {
        std::cout << some.ToString() << more.ToString() << moremore.ToString() << std::endl;
        return some.ToString() + more.ToString() + moremore.ToString();
    }

    BEGIN_REQUEST_MAP()
    ADD_HANDLE(SayHello)
    ADD_HANDLE1(SaySomething)
    ADD_HANDLE3(SayMore)
    END_REQUEST_MAP()
};
} // namespace rpc
