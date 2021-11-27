#pragma once

#include <string>

#include "engine/logger.hpp"
#include "engine/vm.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "modules/openvas/support/scriptstorage.hpp"

char* read_file_content(const char* path, int* file_size);

using namespace Interpreter;

class DefaultExecutorCallback : public Interpreter::ExecutorCallback {
public:
    bool mSyntaxError;

protected:
    std::string mFolder;

public:
    DefaultExecutorCallback(std::string folder) : mFolder(folder) {}

    void OnScriptWillExecute(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        vm->RequireScript("nasl.sc", ctx);
    }
    void OnScriptExecuted(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = mFolder + name;
        if (std::string(name) == "nasl.sc") {
            path = "../script/nasl.sc";
        }
        int iSize = 0;
        void* ptr = read_file_content(path.c_str(), &iSize);
        size = iSize;
        return ptr;
    }
    void OnScriptError(Interpreter::Executor* vm, const char* name, const char* msg) {
        std::string error = msg;
        if (error.find("syntax error") != std::string::npos) {
            mSyntaxError = true;
        }
        LOG(std::string(name) + " " + msg);
    }
};

class HostsTask {
protected:
    struct TCB {
        size_t ThreadHandle;
        std::string Host;
        bool Exit;
        HostsTask* Task;
        int ScriptCount;
        Value Env;
        int ExecutedScriptCount;
        scoped_refptr<openvas::ScriptStorage> Storage;
        TCB(std::string host) {
            Storage = new openvas::ScriptStorage();
            Exit = false;
            ScriptCount = 0;
            ExecutedScriptCount = 0;
            Host = host;
        }
    };
    int mScriptCount;
    std::string mHosts;
    std::string mPorts;
    Value mPrefs;
    Value mGroupedScripts[11];

    std::list<TCB*> mTCBGroup;
    std::string mTaskID;
    size_t mMainThread;

public:
    HostsTask(std::string host, std::string ports, Value& prefs);

public:
    bool BeginTask(std::list<std::string>& scripts, std::string TaskID);
    std::string GetTaskID() { return mTaskID; }

    void Stop();

    bool IsRuning() { return mMainThread != 0; }

protected:
    void Execute();
    void ExecuteOneHost(TCB* tcb);
    void ExecuteScriptOnHost(TCB* tcb);
    static void ExecuteOneHostThreadProxy(void* p) {
        TCB* tcb = (TCB*)p;
        tcb->Task->ExecuteOneHost(tcb);
    }
    static void ExecuteThreadProxy(void* p) {
        HostsTask* ptr = (HostsTask*)p;
        ptr->Execute();
    }
    bool InitScripts(std::list<std::string>& scripts);
    bool InitScripts(openvas::NVTIDataBase& db, std::list<std::string>& scripts,
                     std::map<std::string, int>& loaded);
};
