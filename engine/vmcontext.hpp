#pragma once
#include <map>
#include <string>
#include <vector>

#include "script.hpp"
#include "value.hpp"

namespace Interpreter {

#ifdef _DEBUG_SCRIPT
void DebugContext();

#endif

class VMContext : public CRefCountedThreadSafe<VMContext> {
public:
    enum Type {
        File,
        Function,
        For,
        Switch,
    };
#ifdef _DEBUG_SCRIPT
    void DebugContext();
    static VMContext* sLastContext;

#endif

protected:
    enum {
        CONTINUE_FLAG = (1 << 0),
        BREAK_FLAG = (1 << 1),
        RETURN_FLAG = (1 << 2),
        EXIT_FLAG = (1 << 3),
    };

    std::string mName;
    VMContext* mParent;
    Type mType;
    int mDeepth;
    int mFlags;
    bool mIsEnableWarning;
    Value mReturnValue;

public:
    explicit VMContext(Type type, VMContext* Parent, std::string Name);
    ~VMContext();

    void SetEnableWarning(bool val) { mIsEnableWarning = val; }

    bool IsLocalContext() { return mType == For || mType == Switch; }
    bool IsTop() { return mParent == NULL; }
    bool IsExecutedInterupt() { return (mFlags & 0xFF); }
    void CleanContinueFlag() { mFlags &= 0xFE; }
    void BreakExecuted();
    void ContinueExecuted();
    void ExitExecuted(Value exitCode);
    void ReturnExecuted(Value retVal);
    Value GetReturnValue() { return mReturnValue; }

    bool IsReturnAvaliable() { return IsInFunctionContext(); }
    bool IsBreakAvaliable() { return mType == For || mType == Switch; }
    bool IsContinueAvaliable() { return mType == For; }
    bool IsInFunctionContext();
    bool IsInFunctionContext(std::string&name);

    void AddVar(const std::string& name);
    void SetVarValue(const std::string& name, Value value);
    bool GetVarValue(const std::string& name, Value& val);
    Value GetVarValue(const std::string& name);
    void AddFunction(const Instruction* function);
    const Instruction* GetFunction(const std::string& name);

    Value GetTotalFunction();

    std::string DumpContext(bool global_var = false);

    VMContext* GetTopContext();

protected:
    void LoadBuiltinVar();
    bool IsBuiltinVarName(const std::string& name);
    bool IsShadowName(const std::string& name);

private:
    std::map<std::string, Value> mVars;
    std::map<std::string, const Instruction*> mFunctions;
    DISALLOW_COPY_AND_ASSIGN(VMContext);
};

} // namespace Interpreter