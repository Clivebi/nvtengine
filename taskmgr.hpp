#pragma once

#include <mutex>
#include <string>

#include "engine/logger.hpp"
#include "engine/vm.hpp"
#include "fileio.hpp"
#include "filepath.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "modules/openvas/support/prefsdb.hpp"
#include "modules/openvas/support/scriptstorage.hpp"

using namespace Interpreter;
#define ALGINTO(a, b) (((a / b) + 1) * b)
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

    void OnScriptWillExecute(Interpreter::Executor* vm,
                             scoped_refptr<const Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        ctx->SetVarValue("description", Value(mDescription));
        ctx->SetVarValue("COMMAND_LINE", Value(false));
        vm->RequireScript("nasl.sc", ctx);
    }
    virtual void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script,
                                       VMContext* ctx) {}
    void OnScriptExecuted(Interpreter::Executor* vm,
                          scoped_refptr<const Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = (mFolder + FilePath(name));
        if (std::string(name) == "nasl.sc" || std::string(name) == "servicedetect.sc") {
            path = "../script/" + std::string(name);
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
        LOG_ERROR(std::string(name) + " " + msg);
    }
};

class ScriptCacheImplement : public ScriptCache {
protected:
    std::mutex mLock;
    std::map<std::string, scoped_refptr<Script>> mCache;
    scoped_refptr<Script> mLast;
    Interpreter::Instruction::keyType mNextInsKey;
    Interpreter::Instruction::keyType mNextConstKey;

public:
    ScriptCacheImplement()
            : mLock(), mCache(), mNextInsKey(0x80000000), mNextConstKey(0x80000000), mLast(NULL) {}
    void OnNewScript(scoped_refptr<Script> Script) {
        mLock.lock();
        static const char knownCache[][30] = {
                "nasl.sc",
                "http_func.inc.sc",
                "http_keepalive.inc.sc",
        };
        for (size_t i = 0; i < sizeof(knownCache) / sizeof(knownCache[1]); i++) {
            if (knownCache[i] == Script->Name) {
                Script->RelocateInstruction(mNextInsKey, mNextInsKey);
                mNextInsKey = ALGINTO(Script->GetNextInstructionKey() + 1, 1000);
                mNextConstKey = ALGINTO(Script->GetNextConstKey() + 1, 1000);
                mCache[Script->Name] = Script;
            }
        }
        mLast = Script;
        mLock.unlock();
    }
    scoped_refptr<const Script> GetScriptFromName(const char* name) {
        scoped_refptr<Script> ret = NULL;
        mLock.lock();
        if (mLast != NULL && mLast->Name == name) {
            ret = mLast;
        } else {
            auto item = mCache.find(name);
            if (item != mCache.end()) {
                ret = item->second;
            }
        }
        mLock.unlock();
        return ret;
    }
};

class HostsTask {
protected:
    friend class DetectServiceCallback;
    struct TCB {
        size_t ThreadHandle;
        std::string Host;
        bool Exit;
        HostsTask* Task;
        int ScriptProgress;
        Value Env;
        int ExecutedScriptCount;
        std::vector<int> TCPPorts;
        std::vector<int> UDPPorts;
        scoped_refptr<support::ScriptStorage> Storage;
        TCB(std::string host) {
            Storage = new support::ScriptStorage();
            Env = Value::make_map();
            Exit = false;
            ScriptProgress = 0;
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
    ScriptCacheImplement mScriptCache;

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

protected:
    struct DetectServiceParamter {
        TCB* tcb;
        std::vector<int> ports;
        scoped_refptr<support::ScriptStorage> storage;
    };
    static void DetectServiceProxy(void* p) {
        DetectServiceParamter* param = (DetectServiceParamter*)p;
        param->tcb->Task->DetectService(param);
    }
    void DetectService(DetectServiceParamter* p);
    bool InitScripts(std::list<std::string>& scripts);
    bool InitScripts(support::NVTIDataBase& nvtiDB, support::Prefs& prefsDB,
                     std::list<std::string>& scripts, std::list<Value>& loadOrder,
                     std::map<std::string, int>& loaded);
    bool CheckScript(OVAContext* ctx, Value& nvti);

    void TCPDetectService(TCB* tcb, const std::vector<int>& ports, int thread_count);
};
