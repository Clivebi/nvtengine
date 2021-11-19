#pragma once
#include <map>
#include <string>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "value.hpp"
#include "vmcontext.hpp"
#define VERSION ("0.1")
namespace Interpreter {

class Executor;

typedef struct _BuiltinMethod {
    std::string name;
    RUNTIME_FUNCTION func;
} BuiltinMethod;

class ExecutorCallback {
public:
    virtual void OnScriptWillExecute(Executor* vm, scoped_refptr<Script> Script,
                                     VMContext* ctx) = 0;
    virtual void OnScriptExecuted(Executor* vm, scoped_refptr<Script> Script, VMContext* ctx) = 0;
    virtual void* LoadScriptFile(Executor* vm, const char* name, size_t& size) = 0;
    virtual void OnScriptError(Executor* vm, const char* msg) = 0;
};

class Executor {
public:
    Executor(ExecutorCallback* callback);

public:
    bool Execute(const char* name, bool showWarning = false);
    void RegisgerFunction(BuiltinMethod methods[], int count);
    Value CallScriptFunction(const std::string& name, std::vector<Value>& value, VMContext* ctx);
    void RequireScript(const std::string& name, VMContext* ctx);
    Value GetAvailableFunction(VMContext* ctx);
    void OnScriptError(std::string error);

protected:
    scoped_refptr<Script> LoadScript(const char* name);
    Value Execute(const Instruction* ins, VMContext* ctx);
    Value ExecuteList(std::vector<const Instruction*> insList, VMContext* ctx);
    Value CallFunction(const Instruction* ins, VMContext* ctx);
    Value CallRutimeFunction(const Instruction* ins, VMContext* ctx, RUNTIME_FUNCTION method);
    Value CallScriptFunction(const Instruction* ins, VMContext* ctx, const Instruction* func);
    Value CallScriptFunctionWithNamedParameter(const Instruction* ins, VMContext* ctx,
                                               const Instruction* func);
    Value ExecuteIfStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteForStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteForInStatement(const Instruction* ins, VMContext* ctx);
    Value ExecuteBinaryOperation(const Instruction* ins, VMContext* ctx);
    Value UpdateVar(const std::string& name, Value val, Instructions::Type opCode, VMContext* ctx);
    Value ExecuteUpdateObjectVar(const Instruction* ins, VMContext* ctx);
    Value ExecuteReadObjectVar(const Instruction* ins, VMContext* ctx);
    Value ExecuteCreateMap(const Instruction* ins, VMContext* ctx);
    Value ExecuteCreateArray(const Instruction* ins, VMContext* ctx);
    Value ExecuteSlice(const Instruction* ins, VMContext* ctx);
    Value ExecuteSwitchStatement(const Instruction* ins, VMContext* ctx);
    Value GetFunction(const std::string& name, VMContext* ctx);
    Value UpdateValueAt(Value& toObject, const Value& index, const Value& val, Instructions::Type opCode);
    RUNTIME_FUNCTION GetBuiltinMethod(const std::string& name);
    const Instruction* GetInstruction(Instruction::keyType key);
    std::vector<const Instruction*> GetInstructions(std::vector<Instruction::keyType> keys);
    Value GetConstValue(Instruction::keyType key);
    std::vector<Value> InstructionToValue(std::vector<const Instruction*> ins, VMContext* ctx);
    std::vector<Value> ObjectPathToIndexer(const Instruction* ins, VMContext* ctx);

protected:
    ExecutorCallback* mCallback;
    std::list<scoped_refptr<Script>> mScriptList;
    std::map<std::string, RUNTIME_FUNCTION> mBuiltinMethods;
};
} // namespace Interpreter