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
size_t VMContext::sTlsIndex = 0;

void DumpContext() {
    if (VMContext::sTlsIndex != 0) {
        VMContext* ptr = (VMContext*)TLS::GetValue(VMContext::sTlsIndex);
        if (ptr) {
            LOG_DEBUG(ptr->DumpContext(true));
        }
    }
}

void DumpShortStack(){
    if (VMContext::sTlsIndex != 0) {
        VMContext* ptr = (VMContext*)TLS::GetValue(VMContext::sTlsIndex);
        if (ptr) {
            LOG_DEBUG(ptr->ShortStack());
        }
    }
}

#endif

VMContext::VMContext(Type type, VMContext* Parent, int timeout_second, std::string Name)
        : mFlags(0), mIsEnableWarning(false), mObjectCreator() {
    mParent = Parent;
    mType = type;
    mDeepth = 0;
    mName = Name;
    if (mParent != NULL) {
        mDeepth = mParent->mDeepth + 1;
        mIsEnableWarning = mParent->mIsEnableWarning;
        mDeadTime = 0;
    } else {
        if (timeout_second) {
            mDeadTime = time(NULL) + timeout_second;
        } else {
            mDeadTime = 0;
        }
    }
    Status::sVMContextCount++;
    LoadBuiltinVar();
#ifdef _DEBUG_SCRIPT
    if (VMContext::sTlsIndex != 0) {
        TLS::SetValue(VMContext::sTlsIndex, this);
    }
#endif
}
VMContext::~VMContext() {
    Status::sVMContextCount--;
#ifdef _DEBUG_SCRIPT
    if (VMContext::sTlsIndex != 0) {
        TLS::SetValue(VMContext::sTlsIndex, mParent);
    }
#endif
    for (auto iter : mObjectCreator) {
        delete iter.second;
    }
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

bool VMContext::IsInFunctionContext(std::string& name) {
    VMContext* seek = this;
    while (seek) {
        if (seek->mType == Function) {
            name = seek->mName;
            return true;
        }
        seek = seek->mParent;
    }
    return false;
}

VMContext* VMContext::GetTopContext() {
    VMContext* ctx = this;
    while (ctx->mParent != NULL) {
        ctx = ctx->mParent;
    }
    return ctx;
}

void VMContext::AddVar(const std::string& name) {
    if (!IsBuiltinVarName(name)) {
        throw RuntimeException("variable name is builtin :" + name);
    }
    if (mIsEnableWarning && IsShadowName(name)) {
        LOG_WARNING("variable name shadow :" + name);
    }
    std::map<std::string, Value>::iterator iter = mVars.find(name);
    if (iter == mVars.end()) {
        mVars[name] = Value();
        return;
    }
}

void VMContext::SetVarValue(const std::string& name, Value value, bool current) {
    if (!IsBuiltinVarName(name)) {
        throw RuntimeException("variable not changable,because name is builtin :" + name);
    }
    VMContext* ctx = this;
    bool isInFunction = false;
    while (ctx != NULL) {
        std::map<std::string, Value>::iterator iter = ctx->mVars.find(name);
        if (iter != ctx->mVars.end()) {
            iter->second = value;
            return;
        }
        if (current) {
            ctx->mVars[name] = value;
            return;
        }
        if (ctx->mType == Function) {
            isInFunction = true;
            ctx = GetTopContext();
        } else {
            ctx = ctx->mParent;
        }
    }
    if (isInFunction || NULL == mParent) {
        mVars[name] = value;
        return;
    }
    ctx = GetTopContext();
    ctx->mVars[name] = value;
    //LOG_DEBUG("auto add var in the file context " + name, " File: ", ctx->mName);
}

bool VMContext::IsShadowName(const std::string& name) {
    VMContext* ctx = this;
    if (mType == Function) {
        ctx = GetTopContext();
    } else {
        ctx = mParent;
    }
    while (ctx != NULL) {
        if (ctx->mType == Function) {
            break;
        }
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
        if (ctx->mType == Function) {
            ctx = GetTopContext();
        } else {
            ctx = ctx->mParent;
        }
    }
    return false;
}
Value VMContext::GetVarValue(const std::string& name) {
    Value ret;
    if (!GetVarValue(name, ret)) {
        //DUMP_CONTEXT();
        std::string func;
        IsInFunctionContext(func);
        LOG_DEBUG("variable not found :" + name + " File: " + GetTopContext()->mName + "->", func);
    }
    return ret;
}

void VMContext::AddMethod(const std::string& objectName, const std::string& name,
                          const Instruction* function) {
    std::string func = objectName + "#" + name;
    VMContext* ctx = GetTopContext();
    if (!IsFunctionOverwriteEnabled(func)) {
        throw RuntimeException(func + " function can't overwrite");
    }
    //LOG("add function:"+obj->Name);
    std::map<std::string, const Instruction*>::iterator iter = ctx->mFunctions.find(func);
    if (iter == ctx->mFunctions.end()) {
        ctx->mFunctions[func] = function;
        return;
    }
    throw RuntimeException("function already exist name:" + func);
}

void VMContext::AddFunction(const Instruction* obj) {
    if (mType != File) {
        throw RuntimeException("function declaration must in the top block name:" + obj->Name);
    }
    if (!IsFunctionOverwriteEnabled(obj->Name)) {
        throw RuntimeException(obj->Name + " function can't overwrite");
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
    VMContext* ctx = GetTopContext();

    auto iter = ctx->mFunctions.find(name);
    if (iter != ctx->mFunctions.end()) {
        return iter->second;
    }
    return NULL;
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
            o << prefix << "VARS:\n";
            auto iter2 = ctx->mVars.begin();
            while (iter2 != ctx->mVars.end()) {
                o << prefix << iter2->first << ":" << iter2->second.ToDescription() << "\n";
                iter2++;
            }
        }

        prefix += "\t";
        iter++;
    }
    return o.str();
}

} // namespace Interpreter