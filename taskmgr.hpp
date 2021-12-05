#pragma once

#include <string>

#include "engine/logger.hpp"
#include "engine/vm.hpp"
#include "fileio.hpp"
#include "filepath.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "modules/openvas/support/prefsdb.hpp"
#include "modules/openvas/support/scriptstorage.hpp"

using namespace Interpreter;

class DefaultExecutorCallback : public Interpreter::ExecutorCallback {
public:
    bool mSyntaxError;
    bool mDescription;

protected:
    FilePath mFolder;
    FileIO* mIO;

public:
    DefaultExecutorCallback(FilePath folder, FileIO* IO)
            : mFolder(folder), mDescription(0), mIO(IO) {}

    void OnScriptWillExecute(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        ctx->SetVarValue("description", Value(mDescription));
        ctx->SetVarValue("COMMAND_LINE", Value(false));
        vm->RequireScript("nasl.sc", ctx);
    }
    void OnScriptExecuted(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = (mFolder + FilePath(name));
        if (std::string(name) == "nasl.sc") {
            path = "../script/nasl.sc";
        }
        FileIO io;
        void* ptr = io.Read(path, size);
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
        scoped_refptr<support::ScriptStorage> Storage;
        TCB(std::string host) {
            Storage = new support::ScriptStorage();
            Env = Value::make_map();
            Exit = false;
            ScriptCount = 0;
            ExecutedScriptCount = 0;
            Host = host;
        }
    };

    FileIO* mIO;
    Value mPrefs;
    int mScriptCount;
    std::string mHosts;
    std::string mPorts;
    std::string mTaskID;
    size_t mMainThread;

    std::list<TCB*> mTCBGroup;
    std::list<Value> mGroupedScripts[11];

public:
    HostsTask(std::string host, std::string ports, Value& prefs, FileIO* IO);

public:
    bool BeginTask(std::list<std::string>& scripts, std::string TaskID);
    std::string GetTaskID() { return mTaskID; }

    void Stop();

    void Join();

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
    bool InitScripts(support::NVTIDataBase& nvtiDB, support::Prefs& prefsDB,
                     std::list<std::string>& scripts, std::list<Value>& loadOrder,
                     std::map<std::string, int>& loaded);
    bool CheckScript(OVAContext* ctx, Value& nvti);
};
