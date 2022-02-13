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
#include <pcap.h>

using namespace Interpreter;
#define ALGINTO(a, b) ((((a) / b) + 1) * b)
class DefaultExecutorCallback : public Interpreter::ExecutorCallback {
public:
    bool mSyntaxError;
    bool mDescription;

public:
    DefaultExecutorCallback() : mDescription(false), mSyntaxError(false) {}

    void OnScriptWillExecute(Interpreter::Executor* vm,
                             scoped_refptr<const Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        ctx->SetVarValue("description", Value(mDescription), true);
        ctx->SetVarValue("COMMAND_LINE", Value(false), true);
        ctx->SetVarValue("SCRIPT_NAME", Value(Script->Name), true);
        vm->RequireScript("nasl.sc", ctx);
    }
    virtual void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script,
                                       VMContext* ctx) {}
    void OnScriptExecuted(Interpreter::Executor* vm,
                          scoped_refptr<const Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}

    void OnScriptError(Interpreter::Executor* vm, const std::string& name, const std::string& msg) {
        std::string error = msg;
        if (error.find("syntax error") != std::string::npos) {
            mSyntaxError = true;
        }
        NVT_LOG_ERROR(std::string(name) + " " + msg);
    }
};

class ScriptCacheImplement : public ScriptCache {
protected:
    std::mutex mLock;
    std::map<std::string, scoped_refptr<Script>> mCache;
    Interpreter::Instruction::keyType mNextInsKey;
    Interpreter::Instruction::keyType mNextConstKey;
    DISALLOW_COPY_AND_ASSIGN(ScriptCacheImplement);

public:
    ScriptCacheImplement()
            : mLock(),
              mCache(),
              mNextInsKey(SHARED_SCRIPT_BASE),
              mNextConstKey(SHARED_SCRIPT_BASE) {}
    bool OnNewScript(scoped_refptr<Script> Script) {
        static const char knownCache[][30] = {"nasl.sc", "http_func.inc.sc",
                                              "http_keepalive.inc.sc", "win_base.inc.sc",
                                              "win_warp.inc.sc"};
        bool bRet = false;
        mLock.lock();
        auto item = mCache.find(Script->Name);
        if (item == mCache.end()) {
            for (size_t i = 0; i < sizeof(knownCache) / sizeof(knownCache[1]); i++) {
                if (knownCache[i] == Script->Name) {
                    Script->RelocateInstruction(mNextInsKey, mNextInsKey);
                    mNextInsKey = ALGINTO(Script->GetNextInstructionKey() + 1, 1000);
                    mNextConstKey = ALGINTO(Script->GetNextConstKey() + 1, 1000);
                    mCache[Script->Name] = Script;
                    NVT_LOG_DEBUG("Add Script Cache:", Script->Name);
                    bRet = true;
                }
            }
        }
        mLock.unlock();
        return bRet;
    }
    scoped_refptr<const Script> GetCachedScript(const std::string& name) {
        scoped_refptr<Script> ret = NULL;
        mLock.lock();
        auto item = mCache.find(name);
        if (item != mCache.end()) {
            ret = item->second;
        }
        mLock.unlock();
        return ret;
    }
};

class HostsTask {
protected:
    friend class DetectServiceCallback;
    struct TCB {
        thread_type ThreadHandle;
        std::string Host;
        bool Exit;
        HostsTask* Task;
        int ScriptProgress;
        Value Env;
        std::string Report;
        int ExecutedScriptCount;
        std::vector<int> TCPPorts;
        std::vector<int> UDPPorts;
        scoped_refptr<support::ScriptStorage> Storage;
        time_t BirthTime;
        time_t ScannerTime;
        TCB(std::string host)
                : ThreadHandle(0),
                  Host(host),
                  Exit(false),
                  Task(NULL),
                  ScriptProgress(0),
                  Env(Value::MakeMap()),
                  Report(""),
                  ExecutedScriptCount(0),
                  TCPPorts(),
                  UDPPorts() {
            Storage = new support::ScriptStorage();
            BirthTime = time(NULL);
            ScannerTime = 0;
        }
    };
    ScriptLoader* mLoader;
    Value mPrefs;
    size_t mScriptCount;
    std::string mHosts;
    std::string mPorts;
    std::string mTaskID;
    thread_type mMainThread;
    AtomInt mTaskCount;
    ScriptCacheImplement mScriptCache;

    std::list<TCB*> mTCBGroup;
    std::list<Value> mGroupedScripts[11];
    DISALLOW_COPY_AND_ASSIGN(HostsTask);

public:
    HostsTask(std::string host, std::string ports, Value& prefs, ScriptLoader* IO);

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
    void OutputHostResult(TCB* tcb);
    void OutputJsonResult(TCB* tcb);
    void OutputTextResult(TCB* tcb);
    static void ExecuteOneHostThreadProxy(void* p) {
        TCB* tcb = (TCB*)p;
        tcb->Task->ExecuteOneHost(tcb);
        tcb->Task->mTaskCount--;
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
        DetectServiceParamter() : tcb(NULL), ports(), storage(NULL) {}
    };
    static void DetectServiceProxy(void* p) {
        DetectServiceParamter* param = (DetectServiceParamter*)p;
        param->tcb->Task->DetectService(param);
    }
    void LoadCredential(TCB* tcb);
    void DetectService(DetectServiceParamter* p);
    bool InitScripts(std::list<std::string>& scripts);
    bool InitScripts(support::NVTIDataBase& nvtiDB, support::Prefs& prefsDB,
                     std::list<std::string>& scripts, std::list<Value>& loadOrder,
                     std::map<std::string, int>& loaded);
    void ThinNVTI(Value& nvti, bool lastPhase);
    bool CheckScript(OVAContext* ctx, const Value& nvti);

    void TCPDetectService(TCB* tcb, const std::vector<int>& ports, size_t thread_count);
};
