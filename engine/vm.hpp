#pragma once
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "file.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "value.hpp"
#include "vmcontext.hpp"
#define VERSION ("0.1")
namespace Interpreter {

class Executor;

typedef struct _BuiltinMethod {
    const char* name;
    RUNTIME_FUNCTION func;
} BuiltinMethod;

//shared script object
#define SHARED_SCRIPT_BASE (0x80000000)
class ScriptCache {
public:
    //called cache manger when engine create a new script
    //if this script shared,the cache provider return true
    virtual bool OnNewScript(scoped_refptr<Script> Script) = 0;
    //lookup a shared script object,if not exist ,return NULL
    virtual scoped_refptr<const Script> GetCachedScript(const std::string& name) = 0;
};

//Script loader interface
class ScriptLoader {
public:
    // load script from file by name
    virtual scoped_refptr<Script> LoadScript(const std::string& name, std::string& error) = 0;
    virtual ~ScriptLoader() {}
};

//default script loader implement
class DefaultScriptLoader : public ScriptLoader {
protected:
    FileReader* mReader;
    bool mEncoded;
    DISALLOW_COPY_AND_ASSIGN(DefaultScriptLoader);

    scoped_refptr<Script> LoadScriptFromEncodedFile(const std::string& name, std::string& error);

public:
    explicit DefaultScriptLoader(FileReader* reader, bool encodedFile)
            : mReader(reader), mEncoded(encodedFile) {}
    virtual scoped_refptr<Script> LoadScript(const std::string& name, std::string& error);
};

class ExecutorCallback {
public:
    // call befare main script execute
    virtual void OnScriptWillExecute(Executor* vm, scoped_refptr<const Script> Script,
                                     VMContext* ctx) = 0;
    // call when main script entry executed
    virtual void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script,
                                       VMContext* ctx) = 0;
    // call when script exected
    virtual void OnScriptExecuted(Executor* vm, scoped_refptr<const Script> Script,
                                  VMContext* ctx) = 0;
    // call when script error
    virtual void OnScriptError(Executor* vm, const std::string& name, const std::string& msg) = 0;
};

class Executor {
public:
    Executor(ExecutorCallback* callback, ScriptLoader* Loader);

public:
    bool Execute(const std::string& name, int timeout_second = 0, bool showWarning = false,
                 bool onlyParse = false);

    void RegisgerFunction(BuiltinMethod methods[], int count, std::string prefix = "");

    Value CallScriptFunction(const std::string& name, std::vector<Value>& value, VMContext* ctx);

    Value CallObjectMethod(Value object, Value func, std::vector<Value>& value, VMContext* ctx);

    void RequireScript(const std::string& name, VMContext* ctx);

    Value GetAvailableFunction(VMContext* ctx);

    void* GetUserContext() { return mContext; }

    void SetUserContext(void* ctx) { mContext = ctx; }

    Value GetFunction(const std::string& name, VMContext* ctx);

    void SetScriptCacheProvider(ScriptCache* ptr) { mCacheProvider = ptr; }

protected:
    scoped_refptr<const Script> LoadScript(const std::string& name, std::string& error);
    Value Execute(const Instruction* ins, VMContext* ctx);
    Value ExecuteList(std::vector<const Instruction*> insList, VMContext* ctx);
    Value CallFunction(const Instruction* ins, VMContext* ctx);
    Value CallMethod(const Instruction* ins, VMContext* ctx);
    Value CallRutimeFunction(const Instruction* ins, VMContext* ctx, RUNTIME_FUNCTION method);
    Value CallScriptFunction(const Instruction* ins, VMContext* ctx, const Instruction* func);
    Value ExecuteIfStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteForStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteForInStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteBinaryOperation(const Instruction* ins, VMContext* ctx);
    bool ConvertNilWhenUpdate(Value& oldVal, Value val, Instructions::Type opCode, VMContext* ctx);
    Value UpdateVar(const std::string& name, Value val, Instructions::Type opCode, VMContext* ctx);
    Value ExecuteUpdateObjectVar(const Instruction* ins, VMContext* ctx);
    Value ConvertNil(Value index, VMContext* ctx);
    Value ExecuteReadObjectVar(const Instruction* ins, VMContext* ctx);
    Value ExecuteCreateMap(const Instruction* ins, VMContext* ctx);
    Value ExecuteCreateArray(const Instruction* ins, VMContext* ctx);
    Value ExecuteSlice(const Instruction* ins, VMContext* ctx);
    Value ExecuteSwitchStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteObjectDeclaration(const Instruction* ins, VMContext* ctx);
    Value ExecuteNewObject(VMContext::ObjectCreator* creator, const Instruction* actual,
                           VMContext* ctx);

    Value BatchAddOperation(const Instruction* ins, VMContext* ctx);

    scoped_refptr<VMContext> FillActualParameters(const Instruction* func,
                                                  const Instruction* actual, VMContext* ctx);

    scoped_refptr<VMContext> FillActualParameters(const Instruction* func,
                                                  std::vector<Value>& values, VMContext* ctx);
    bool IsNameInGroup(const std::string& name, std::vector<const Instruction*>& groups) {
        for (auto iter : groups) {
            if (iter->Name == name) {
                return true;
            }
        }
        return false;
    }

    Value UpdateValueAt(Value& toObject, const Value& index, const Value& val,
                        Instructions::Type opCode);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);
    const Instruction* GetInstruction(Instruction::keyType key);
    std::vector<const Instruction*> GetInstructions(std::vector<Instruction::keyType> keys);
    Value GetConstValue(Instruction::keyType key);
    std::vector<Value> InstructionToValue(std::vector<const Instruction*> ins, VMContext* ctx);
    std::vector<Value> ObjectPathToIndexer(const Instruction* ins, VMContext* ctx);

    void Reset() {
        mCurrentVMContxt = NULL;
        mScripts.clear();
        mSharedScripts.clear();
    }

protected:
    void* mContext;
    VMContext* mCurrentVMContxt;
    ExecutorCallback* mCallback;
    ScriptLoader* mLoader;
    ScriptCache* mCacheProvider;
    std::list<scoped_refptr<Script>> mScripts;
    std::list<scoped_refptr<const Script>> mSharedScripts;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};

void InitializeLibray();
} // namespace Interpreter