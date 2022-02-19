#pragma once
#include "jsonrpc.hpp"
#include "modules/net/con.hpp"
class MangerServer : rpc::BaseJsonRpcHandler {
protected:
    void OnNewConnection(scoped_refptr<net::Conn> con);
    struct tcp_worker_thread_paramters {
        MangerServer* pServer;
        scoped_refptr<net::Conn> con;
        tcp_worker_thread_paramters(MangerServer* p, scoped_refptr<net::Conn> con)
                : pServer(p), con(con) {}
    };

    static void tcp_worker_thread(void* p) {
        tcp_worker_thread_paramters* arg = (tcp_worker_thread_paramters*)p;
        arg->pServer->OnNewConnection(arg->con);
        delete p;
    }

public:
    void AsyncNewConnection(scoped_refptr<net::Conn> con);

protected:
    Value BeginTask(Value& host, Value& port, Value& pref, Value& scriptSelector);
    Value StopTask(Value& taskID);
    Value GetTaskInformation(Value& taskID);
    BEGIN_REQUEST_MAP()
    END_REQUEST_MAP()
};

void TCPForever(MangerServer* srv, std::string port, std::string host = "localhost");
