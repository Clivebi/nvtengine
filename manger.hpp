#pragma once
#include "jsonrpc.hpp"
#include "modules/net/con.hpp"
#include "ntvpref.hpp"
#include "taskmgr.hpp"
class RPCServer : rpc::BaseJsonRpcHandler {
protected:
    void OnNewConnection(scoped_refptr<net::Conn> con);
    struct tcp_worker_thread_paramters {
        RPCServer* pServer;
        scoped_refptr<net::Conn> con;
        tcp_worker_thread_paramters(RPCServer* p, scoped_refptr<net::Conn> con)
                : pServer(p), con(con) {}
    };

    static void tcp_worker_thread(void* p) {
        tcp_worker_thread_paramters* arg = (tcp_worker_thread_paramters*)p;
        arg->pServer->OnNewConnection(arg->con);
        delete arg;
    }

public:
    void AsyncNewConnection(scoped_refptr<net::Conn> con);
    RPCServer(Value& pref, ScriptLoader* Loader)
            : mTasks(), mLock(), mPref(pref), mLoader(Loader) {}
    ~RPCServer() {
        mLock.lock();
        for (auto iter : mTasks) {
            iter.second->Stop();
        }
        for (auto iter : mTasks) {
            iter.second->Join();
            delete iter.second;
        }
        mLock.unlock();
    }

protected:
    Value BeginTask(Value& host, Value& port, Value& filter, Value& taskID);
    Value StopTask(Value& taskID);
    Value CleanTask(Value& taskID);
    Value GetTaskStatus(Value& taskID);
    Value EnumTask();
    Value GetDiscoverdHost(Value& taskID);
    Value GetFinishedHost(Value& taskID);
    Value GetHostTaskInformation(Value& taskID, Value& host);
    Value StopHostTask(Value& taskID, Value& host);
    Value GetEngineVersion();
    BEGIN_REQUEST_MAP()
    ADD_HANDLE(GetEngineVersion)
    ADD_HANDLE(EnumTask)
    ADD_HANDLE4(BeginTask)
    ADD_HANDLE1(StopTask)
    ADD_HANDLE1(CleanTask)
    ADD_HANDLE1(GetTaskStatus)
    ADD_HANDLE1(GetDiscoverdHost)
    ADD_HANDLE1(GetFinishedHost)
    ADD_HANDLE2(GetHostTaskInformation)
    ADD_HANDLE2(StopHostTask)
    END_REQUEST_MAP()
protected:
    Value mPref;
    std::mutex mLock;
    ScriptLoader* mLoader;
    std::map<std::string, HostsTask*> mTasks;

    HostsTask* LookupTask(std::string taskID) {
        HostsTask* ptr = NULL;
        mLock.lock();
        auto iter = mTasks.find(taskID);
        if (iter != mTasks.end()) {
            ptr = iter->second;
        }
        mLock.unlock();
        return ptr;
    }
    void AddTask(std::string taskID, HostsTask* task) {
        mLock.lock();
        mTasks[taskID] = task;
        mLock.unlock();
    }
    void LoadOidList(std::string filter, std::list<std::string>& result);
};

void ServeTCP(RPCServer* srv, std::string port, std::string host = "localhost");
