#pragma once

#include <string>

#include "engine/logger.hpp"
#include "engine/vm.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "modules/openvas/support/scriptstorage.hpp"

class FilePath {
protected:
    std::string _full;
#ifdef WIN32
    static const char s_separator = '\\';
#else
    static const char s_separator = '/';
#endif
public:
    FilePath(const char* src) : _full(src) {}
    FilePath(std::string src) : _full(src) {}

    FilePath& operator+=(const FilePath& part) {
        if (part._full.size() == 0) {
            return *this;
        }
        if (_full.size() && _full.back() != s_separator && part._full[0] != s_separator) {
            _full += s_separator;
        }
        _full += part._full;
        return *this;
    }
    FilePath operator+(const FilePath& right) {
        FilePath ret = *this;
        ret += right;
        return ret;
    }

    FilePath& operator+=(const char* part) {
        std::string right(part);
        return this->operator+=(right);
    }

    operator std::string() { return _full; }
    std::string base_name() {
        std::string part = _full;
        if (part.size() && part.back() == s_separator) {
            part = _full.substr(0, part.size() - 1);
        }
        size_t i = part.rfind(s_separator);
        if (i == part.npos) {
            return part;
        }
        return part.substr(i);
    }

    std::string extension_name() {
        size_t i = _full.rfind('.');
        if (i == _full.npos || i >= _full.size()) {
            return "";
        }
        return _full.substr(i + 1);
    }
};

char* read_file_content(const char* path, int* file_size);

using namespace Interpreter;

class DefaultExecutorCallback : public Interpreter::ExecutorCallback {
public:
    bool mSyntaxError;
    bool mDescription;

protected:
    FilePath mFolder;

public:
    DefaultExecutorCallback(FilePath folder) : mFolder(folder),mDescription(0) {}

    void OnScriptWillExecute(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        ctx->SetVarValue("description",Value(mDescription));
        vm->RequireScript("nasl.sc", ctx);
    }
    void OnScriptExecuted(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = (mFolder+FilePath(name));
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
            Env = Value::make_map();
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
    bool InitScripts(openvas::NVTIDataBase& db, std::list<std::string>& scripts,
                     std::map<std::string, int>& loaded);
};
