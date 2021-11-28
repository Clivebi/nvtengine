#include "vmcontext.hpp"
#include <sstream>
bool IsFunctionOverwriteEnabled(const std::string& name);
namespace Interpreter {

struct BuiltinValue {
    std::string Name;
    Value val;
};

BuiltinValue g_builtinVar[] = {
        {"false", Value(0l)},
        {"true", Value(1l)},
        {"nil", Value()},
};

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

#ifdef _DEBUG_SCRIPT
VMContext* VMContext::sLastContext = NULL;

void DebugContext() {
    if (VMContext::sLastContext != NULL) {
        LOG(VMContext::sLastContext->DumpContext(true));
    }
}
#endif

VMContext::VMContext(Type type, VMContext* Parent, std::string Name)
        : mFlags(0), mIsEnableWarning(false) {
    mParent = Parent;
    mType = type;
    mDeepth = 0;
    mName = Name;
    if (mParent != NULL) {
        mDeepth = mParent->mDeepth + 1;
        mIsEnableWarning = mParent->mIsEnableWarning;
    }
    Status::sVMContextCount++;
    LoadBuiltinVar();
#ifdef _DEBUG_SCRIPT
    VMContext::sLastContext = this;
#endif
}
VMContext::~VMContext() {
    Status::sVMContextCount--;
#ifdef _DEBUG_SCRIPT
    VMContext::sLastContext = mParent;
#endif
}

bool VMContext::IsBuiltinVarName(const std::string& name) {
    for (int i = 0; i < COUNT_OF(g_builtinVar); i++) {
        if (g_builtinVar[i].Name == name) {
            return false;
        }
    }
    return true;
}

void VMContext::LoadBuiltinVar() {
    for (int i = 0; i < COUNT_OF(g_builtinVar); i++) {
        mVars[g_builtinVar[i].Name] = g_builtinVar[i].val;
    }
}

void VMContext::BreakExecuted() {
    if (!IsBreakAvaliable()) {
        throw RuntimeException("break only avaliable in for or switch statement");
    }
    mFlags |= BREAK_FLAG;
}

void VMContext::ContinueExecuted() {
    if (!IsContinueAvaliable()) {
        throw RuntimeException("continue only avaliable in for statement");
    }
    mFlags |= CONTINUE_FLAG;
}

void VMContext::ExitExecuted(Value exitCode) {
    VMContext* seek = this;
    while (seek) {
        seek->mFlags |= EXIT_FLAG;
        seek->mReturnValue = exitCode;
        seek = seek->mParent;
    }
}

void VMContext::ReturnExecuted(Value retVal) {
    if (!IsInFunctionContext()) {
        throw RuntimeException("return only avaliable in function statement");
    }
    VMContext* seek = this;
    while (seek) {
        seek->mFlags |= RETURN_FLAG;
        seek->mReturnValue = retVal;
        if (seek->mType == Function) {
            break;
        }
        seek = seek->mParent;
    }
}

bool VMContext::IsInFunctionContext() {
    VMContext* seek = this;
    while (seek) {
        if (seek->mType == Function) {
            return true;
        }
        seek = seek->mParent;
    }
    return false;
}

void VMContext::AddVar(const std::string& name) {
    if (!IsBuiltinVarName(name)) {
        throw RuntimeException("variable name is builtin :" + name);
    }
    if (mIsEnableWarning && IsShadowName(name)) {
        LOG("variable name shadow :" + name);
    }
    std::map<std::string, Value>::iterator iter = mVars.find(name);
    if (iter == mVars.end()) {
        mVars[name] = Value();
        return;
    }
}

void VMContext::SetVarValue(const std::string& name, Value value) {
    if (!IsBuiltinVarName(name)) {
        throw RuntimeException("variable not changable,because name is builtin :" + name);
    }
    VMContext* ctx = this;
    while (ctx != NULL) {
        std::map<std::string, Value>::iterator iter = ctx->mVars.find(name);
        if (iter != ctx->mVars.end()) {
            iter->second = value;
            return;
        }
        ctx = ctx->mParent;
    }
    if (mIsEnableWarning) {
        LOG("variable <" + name + "> not found, so new one.");
    }
    mVars[name] = value;
}

bool VMContext::IsShadowName(const std::string& name) {
    VMContext* ctx = this;
    while (ctx != NULL) {
        std::map<std::string, Value>::iterator iter = ctx->mVars.find(name);
        if (iter != ctx->mVars.end()) {
            return true;
        }
        ctx = ctx->mParent;
    }
    return false;
}

bool VMContext::GetVarValue(const std::string& name, Value& val) {
    VMContext* ctx = this;
    while (ctx != NULL) {
        std::map<std::string, Value>::iterator iter = ctx->mVars.find(name);
        if (iter != ctx->mVars.end()) {
            val = iter->second;
            return true;
        }
        ctx = ctx->mParent;
    }
    return false;
}
Value VMContext::GetVarValue(const std::string& name) {
    Value ret;
    if (!GetVarValue(name, ret)) {
        DEBUG_CONTEXT();
        throw RuntimeException("variable not found :" + name);
    }
    return ret;
}

void VMContext::AddFunction(const Instruction* obj) {
    if (mType != File) {
        throw RuntimeException("function declaration must in the top block name:" + obj->Name);
    }
    if (!IsFunctionOverwriteEnabled(obj->Name)) {
        throw RuntimeException("exit function can't overwrite");
    }
    //LOG("add function:"+obj->Name);
    std::map<std::string, const Instruction*>::iterator iter = mFunctions.find(obj->Name);
    if (iter == mFunctions.end()) {
        mFunctions[obj->Name] = obj;
        return;
    }
    throw RuntimeException("function already exist name:" + obj->Name);
}

const Instruction* VMContext::GetFunction(const std::string& name) {
    VMContext* ctx = this;
    while (ctx->mParent != NULL) {
        ctx = ctx->mParent;
    }
    std::map<std::string, const Instruction*>::iterator iter = ctx->mFunctions.find(name);
    if (iter == ctx->mFunctions.end()) {
        return NULL;
    }
    return iter->second;
}

Value VMContext::GetTotalFunction() {
    VMContext* ctx = this;
    while (ctx->mParent != NULL) {
        ctx = ctx->mParent;
    }
    std::vector<Value> functions;
    std::map<std::string, const Instruction*>::iterator iter = ctx->mFunctions.begin();
    while (iter != ctx->mFunctions.end()) {
        functions.push_back(Value(iter->first));
        iter++;
    }
    return Value(functions);
}
std::string VMContext::DumpContext(bool var) {
    std::list<VMContext*> list;
    std::stringstream o;
    VMContext* ctx = this;
    list.push_front(ctx);
    while (ctx->mParent != NULL) {
        ctx = ctx->mParent;
        list.push_front(ctx);
    }
    std::string prefix = "";
    auto iter = list.begin();
    while (iter != list.end()) {
        ctx = (*iter);
        o << prefix << "Name:" << ctx->mName << " Depth:" << ctx->mDeepth << " Type:";
        switch (ctx->mType) {
        case Type::File:
            o << "File";
            break;
        case Type::Function:
            o << "Function";
            break;
        case Type::For:
            o << "For";
            break;
        case Type::Switch:
            o << "Switch";
            break;

        default:
            break;
        }
        o << "\n";
        if (var || ctx->mType == Function) {
            o << prefix << "Vars:\n";
            auto iter2 = ctx->mVars.begin();
            while (iter2 != ctx->mVars.end()) {
                o << prefix << iter2->first << ":" << iter2->second.ToString() << "\n";
                iter2++;
            }
        }

        prefix += "\t";
        iter++;
    }
    return o.str();
}

} // namespace Interpreter