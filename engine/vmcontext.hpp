#pragma once
#include <map>
#include <string>
#include <vector>

#include "script.hpp"
#include "tls.hpp"
#include "value.hpp"

namespace Interpreter {

#ifdef _DEBUG_SCRIPT
void DumpContext();

void DumpShortStack();

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
    static size_t sTlsIndex;
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
    time_t mDeadTime;
    int mDeepth;
    int mFlags;
    bool mIsEnableWarning;
    Value mReturnValue;

public:
    explicit VMContext(Type type, VMContext* Parent, int timeout_second, std::string Name);
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
    bool IsInFunctionContext(std::string& name);

    bool IsTimeout() {
        time_t now = time(NULL);
        time_t end = GetTopContext()->mDeadTime;
        if (end == 0) {
            return false;
        }
        return now > end;
    }

    void AddVar(const std::string& name);
    void SetVarValue(const std::string& name, Value value, bool current = false);
    bool GetVarValue(const std::string& name, Value& val);
    Value GetVarValue(const std::string& name);
    void AddFunction(const Instruction* function);
    void AddMethod(const std::string& objectName, const std::string& name,
                   const Instruction* function);
    const Instruction* GetFunction(const std::string& name);

    Value GetTotalFunction();

    std::string DumpContext(bool global_var = false);

    VMContext* GetTopContext();

    struct ObjectCreator {
        bool Initialzed;
        std::string Name;
        std::map<std::string, Value> Attributes;
        const Instruction* MethodsList;
    };

    void AddObjectCreator(const std::string name, ObjectCreator* creator) {
        GetTopContext()->mObjectCreator[name] = creator;
    }

    ObjectCreator* GetObjectCreator(const std::string name) {
        VMContext* ctx = GetTopContext();
        auto iter = ctx->mObjectCreator.find(name);
        if (iter != ctx->mObjectCreator.end()) {
            return iter->second;
        }
        return NULL;
    }

    std::string ShortDescription() {
        switch (mType) {
        case File:
            return "File:" + mName;
        case For:
            return "For";
        case Function:
            return "Function:" + mName;
        case Switch:
            return "Switch";
        }
        return "";
    }

    std::string ShortStack() {
        std::string stacks = "";
        VMContext* ptr = this;
        while (ptr) {
            stacks = ptr->ShortDescription() + "-->" + stacks;
            ptr = ptr->mParent;
        }
        return stacks;
    }

protected:
    void LoadBuiltinVar();
    bool IsBuiltinVarName(const std::string& name);
    bool IsShadowName(const std::string& name);

    std::map<std::string, Value> mVars;
    std::map<std::string, const Instruction*> mFunctions;
    std::map<std::string, ObjectCreator*> mObjectCreator;
    DISALLOW_COPY_AND_ASSIGN(VMContext);
};

} // namespace Interpreter