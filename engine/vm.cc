#include "vm.hpp"

#include "logger.hpp"
#include "parser.hpp"
using namespace Interpreter;
#include "script.lex.hpp"
#include "script.tab.hpp"

#define ALGINTO(a, b) ((((a) / b) + 1) * b)

#ifdef _DEBUG_MEMORY_BROKEN
void* operator new(size_t sz) {
    BYTE* ptr = (BYTE*)malloc(sz + 64);
    for (size_t i = 0; i < 34; i++) {
        ptr[i] = 0xCC;
    }
    for (size_t i = sz; i < 30; i++) {
        ptr[i] = 0xCC;
    }
    *(uint32_t*)ptr = sz;
    return ptr + 34;
}

void operator delete(void* p) noexcept {
    BYTE* ptr = (BYTE*)p;
    ptr -= 34;
    uint32_t sz = *(uint32_t*)ptr;
    for (size_t i = 4; i < 34; i++) {
        if (ptr[i] != 0xCC) {
            abort();
        }
    }

    for (size_t i = sz; i < 30; i++) {
        if (ptr[i] != 0xCC) {
            abort();
        }
    }
    free(ptr);
}
#endif

int g_LogLevel = 3;

void RegisgerEngineBuiltinMethod(Interpreter::Executor* vm);

void yyerror(Interpreter::Parser* parser, const char* s) {
    char message[1024] = {0};
    sprintf(message, "%s on line:%d column:%d ", s, yyget_lineno(parser->GetContext()),
            yyget_column(parser->GetContext()));
    parser->mLastError = message;
}

void RegisgerEngineBuiltinMethod(Interpreter::Executor* vm);

namespace Interpreter {

Executor::Executor(ExecutorCallback* callback, void* userContext)
        : mScripts(), mSharedScripts(), mCallback(callback), mCacheProvider(NULL) {
    mContext = userContext;
    RegisgerEngineBuiltinMethod(this);
}

bool Executor::Execute(const char* name, int timeout_second, bool showWarning) {
    bool bRet = false;
    std::string error = "";
    Reset();
    scoped_refptr<const Script> script = LoadScript(name, error);
    if (script == NULL) {
        mCallback->OnScriptError(this, name, error.c_str());
        return false;
    }
    scoped_refptr<VMContext> context = new VMContext(VMContext::File, NULL, timeout_second, name);
    mCurrentVMContxt = context;
    context->SetEnableWarning(showWarning);

    try {
        mCallback->OnScriptWillExecute(this, script, context);
        Execute(script->EntryPoint, context);
        bRet = true;
        mCallback->OnScriptEntryExecuted(this, script, context);
    } catch (const RuntimeException& e) {
        error = e.what();
    }
    if (!bRet) {
        mCallback->OnScriptError(this, name, error.c_str());
    }
    mCallback->OnScriptExecuted(this, script, context);
    return bRet;
}

scoped_refptr<const Script> Executor::LoadScript(const char* name, std::string& error) {
    bool bShared = false;
    if (mCacheProvider != NULL) {
        scoped_refptr<const Script> preview = mCacheProvider->GetScriptFromName(name);
        if (preview != NULL) {
            bShared = true;
            mSharedScripts.push_back(preview);
            return preview;
        }
    }
    scoped_refptr<Script> script = LoadScriptInternal(name, error);
    if (script == NULL) {
        return NULL;
    }
    if (mCacheProvider != NULL) {
        if (mCacheProvider->OnNewScript(script)) {
            mSharedScripts.push_back(script);
            bShared = true;
        }
    }
    if (bShared) {
        return script;
    }
    if (mScripts.size() > 0) {
        scoped_refptr<const Script> last = mScripts.back();
        script->RelocateInstruction(ALGINTO((last->GetNextInstructionKey() + 1), 1000),
                                    ALGINTO((last->GetNextConstKey() + 1), 1000));
    }
    mScripts.push_back(script);
    return script;
}

scoped_refptr<Script> Executor::LoadScriptInternal(const char* name, std::string& error) {
    size_t size = 0;
    void* data = mCallback->LoadScriptFile(this, name, size);
    if (data == NULL) {
        error = "callback LoadScriptFile failed";
        return NULL;
    }
    YY_BUFFER_STATE bp;
    yyscan_t scanner;
    yylex_init(&scanner);
    scoped_refptr<Parser> parser = new Parser(scanner);
    bp = yy_scan_bytes((char*)data, (int)size, scanner);
    yy_switch_to_buffer(bp, scanner);
    parser->Start(name);
    int err = yyparse(parser.get());
    yy_flush_buffer(bp, scanner);
    yy_delete_buffer(bp, scanner);
    yylex_destroy(scanner);
    free(data);
    if (err) {
        error = parser->mLastError;
        return NULL;
    }
    return parser->Finish();
}

void Executor::RegisgerFunction(BuiltinMethod methods[], int count, std::string prefix) {
    for (int i = 0; i < count; i++) {
        mBuiltinMethods[prefix + methods[i].name] = methods[i].func;
    }
}

RUNTIME_FUNCTION Executor::GetBuiltinMethod(const std::string& name) {
    std::map<std::string, RUNTIME_FUNCTION>::iterator iter = mBuiltinMethods.find(name);
    if (iter == mBuiltinMethods.end()) {
        return NULL;
    }
    return iter->second;
}

void Executor::RequireScript(const std::string& name, VMContext* ctx) {
    for (auto iter : mScripts) {
        if (iter->Name == name) {
            return;
        }
    }
    for (auto iter : mSharedScripts) {
        if (iter->Name == name) {
            return;
        }
    }
    std::string error;
    scoped_refptr<const Script> required = LoadScript(name.c_str(), error);
    if (required.get() == NULL) {
        throw RuntimeException("load script <" + name + "> failed :" + error);
    }
    Execute(required->EntryPoint, ctx);
}

Value Executor::GetAvailableFunction(VMContext* ctx) {
    std::vector<Value> builtin;
    std::map<std::string, RUNTIME_FUNCTION>::iterator iter = mBuiltinMethods.begin();
    while (iter != mBuiltinMethods.end()) {
        builtin.push_back(Value(iter->first));
        iter++;
    }
    Value result = Value::MakeMap();
    result._map()[Value("script")] = ctx->GetTotalFunction();
    result._map()[Value("builtin")] = builtin;
    return result;
}

const Instruction* Executor::GetInstruction(Instruction::keyType key) {
    if (key >= SHARED_SCRIPT_BASE) {
        for (auto iter : mSharedScripts) {
            if (iter->IsContainInstruction(key)) {
                return iter->GetInstruction(key);
            }
        }
    }
    for (auto iter : mScripts) {
        if (iter->IsContainInstruction(key)) {
            return iter->GetInstruction(key);
        }
    }
    char buf[16] = {0};
    snprintf(buf, 16, "%08X", key);
    throw RuntimeException(std::string("unknown instruction key:") + buf);
}

std::vector<const Instruction*> Executor::GetInstructions(std::vector<Instruction::keyType> keys) {
    if (keys.size() == 0) {
        return std::vector<const Instruction*>();
    }
    if (keys[0] >= SHARED_SCRIPT_BASE) {
        for (auto iter : mSharedScripts) {
            if (iter->IsContainInstruction(keys[0])) {
                return iter->GetInstructions(keys);
            }
        }
    }
    for (auto iter : mScripts) {
        if (iter->IsContainInstruction(keys[0])) {
            return iter->GetInstructions(keys);
        }
    }
    char buf[16] = {0};
    snprintf(buf, 16, "%08X", keys[0]);
    throw RuntimeException(std::string("unknown instructions first key:") + buf);
}

Value Executor::GetConstValue(Instruction::keyType key) {
    if (key >= SHARED_SCRIPT_BASE) {
        for (auto iter : mSharedScripts) {
            if (iter->IsContainConst(key)) {
                return iter->GetConstValue(key);
            }
        }
    }
    for (auto iter : mScripts) {
        if (iter->IsContainConst(key)) {
            return iter->GetConstValue(key);
        }
    }
    char buf[16] = {0};
    snprintf(buf, 16, "%08X", key);
    throw RuntimeException(std::string("unknown const key") + buf);
}

Value Executor::Execute(const Instruction* ins, VMContext* ctx) {
    //LOG("execute " + ins->ToString());
    if (ctx->IsTimeout()) {
        throw RuntimeException("script execute timeout...");
    }
    if (ctx->IsExecutedInterupt()) {
        LOG_DEBUG("Instruction execute interupted :" + ins->ToString());
        return ctx->GetReturnValue();
    }
    if (ins->OpCode >= Instructions::kADD && ins->OpCode <= Instructions::kMAXBinaryOP) {
        return ExecuteBinaryOperation(ins, ctx);
    }
    if ((ins->OpCode & Instructions::kUpdate) == Instructions::kUpdate) {
        return ExecuteUpdateObjectVar(ins, ctx);
    }
    switch (ins->OpCode) {
    case Instructions::kNop:
        return Value();
    case Instructions::kConst:
        return GetConstValue(ins->Refs[0]);
    case Instructions::kNewVar: {
        ctx->AddVar(ins->Name);
        if (ins->Refs.size()) {
            Value initValue = Execute(GetInstruction(ins->Refs[0]), ctx);
            ctx->SetVarValue(ins->Name, initValue);
            return initValue;
        }
        return Value();
    }
    case Instructions::kReadVar:
        return ctx->GetVarValue(ins->Name);
    case Instructions::kMinus: {
        Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
        switch (val.Type) {
        case ValueType::kInteger:
            val.Integer = (-val.Integer);
            return val;
        case ValueType::kFloat:
            val.Float = (-val.Float);
            return val;
        default:
            throw RuntimeException("minus operation only can applay to integer or float " +
                                   ctx->ShortStack());
        }
    }
    case Instructions::kNewFunction: {
        if (ins->Name.size()) {
            ctx->AddFunction(ins);
            return Value();
        }
        return Value(ins);
    }
    case Instructions::kObjectDecl: {
        return ExecuteObjectDeclaration(ins, ctx);
    }
    case Instructions::kCallObjectMethod: {
        return CallMethod(ins, ctx);
    }
    case Instructions::kCallFunction: {
        return CallFunction(ins, ctx);
    }

    case Instructions::kGroup: {
        ExecuteList(GetInstructions(ins->Refs), ctx);
        return Value();
    }

    case Instructions::kIFStatement: {
        ExecuteIfStatement(ins, ctx);
        return Value();
    }

    //return indcate the action executed or not
    case Instructions::kContitionExpression: {
        Value val = Execute(GetInstruction(ins->Refs[0]), ctx);
        if (val.ToBoolean()) {
            Execute(GetInstruction(ins->Refs[1]), ctx);
        }
        return val;
    }

    case Instructions::kRETURNStatement: {
        if (ins->Refs.size() == 1) {
            ctx->ReturnExecuted(Execute(GetInstruction(ins->Refs[0]), ctx));
        } else {
            ctx->ReturnExecuted(Value());
        }
        return ctx->GetReturnValue();
    }

    case Instructions::kFORStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx, 0, "");
        mCurrentVMContxt = newCtx;
        ExecuteForStatement(ins, newCtx);
        mCurrentVMContxt = ctx;
        return Value();
    }
    case Instructions::kForInStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx, 0, "");
        mCurrentVMContxt = newCtx;
        ExecuteForInStatement(ins, newCtx);
        mCurrentVMContxt = ctx;
        return Value();
    }
    case Instructions::kSwitchCaseStatement: {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Switch, ctx, 0, "");
        mCurrentVMContxt = newCtx;
        ExecuteSwitchStatement(ins, newCtx);
        mCurrentVMContxt = ctx;
        return Value();
    }
    case Instructions::kBREAKStatement: {
        ctx->BreakExecuted();
        return Value();
    }
    case Instructions::kCONTINUEStatement: {
        ctx->ContinueExecuted();
        return Value();
    }
    case Instructions::kCreateMap:
        return ExecuteCreateMap(ins, ctx);
    case Instructions::kCreateArray:
        return ExecuteCreateArray(ins, ctx);
    case Instructions::kReadReference:
        return ExecuteReadObjectVar(ins, ctx);
    case Instructions::kSlice:
        return ExecuteSlice(ins, ctx);
    default:
        assert(false);
        LOG_ERROR("Unknown Instruction:" + ins->ToString());
        return Value();
    }
}

std::vector<Value> Executor::ObjectPathToIndexer(const Instruction* ins, VMContext* ctx) {
    assert(ins->OpCode == Instructions::kPath);
    return InstructionToValue(GetInstructions(ins->Refs), ctx);
}

Value Executor::UpdateVar(const std::string& name, Value val, Instructions::Type opCode,
                          VMContext* ctx) {
    Instructions::Type code = opCode & 0xFF;
    if (code == Instructions::kuWrite) {
        ctx->SetVarValue(name, val);
        return val;
    }

    Value oldVal;
    ctx->GetVarValue(name, oldVal);
    if (oldVal.IsNULL()) {
        oldVal.Type = ValueType::kInteger;
        if (val.IsString()) {
            oldVal.Type = val.Type;
        }
    }
    switch (code) {
    case Instructions::kuADD:
        oldVal += val;
        break;
    case Instructions::kuSUB:
        oldVal -= val;
        break;
    case Instructions::kuDIV:
        oldVal *= val;
        break;
    case Instructions::kuMUL:
        oldVal /= val;
        break;
    case Instructions::kuBOR:
        oldVal |= val;
        break;
    case Instructions::kuBAND:
        oldVal &= val;
        break;
    case Instructions::kuBXOR:
        oldVal ^= val;
        break;
    case Instructions::kuLSHIFT:
        oldVal <<= val;
        break;
    case Instructions::kuRSHIFT:
        oldVal >>= val;
        break;
    case Instructions::kuINC:
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("++ operation only can used on Integer " + ctx->ShortStack());
        }
        oldVal.Integer++;
        break;
    case Instructions::kuDEC:
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("-- operation only can used on Integer " + ctx->ShortStack());
        }
        oldVal.Integer--;
        break;
    case Instructions::kuINCReturnOld: {
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("++ operation only can used on Integer " + ctx->ShortStack());
        }
        Value ret = oldVal;
        oldVal.Integer++;
        ctx->SetVarValue(name, oldVal);
        return ret;
    }

    case Instructions::kuDECReturnOld: {
        if (!oldVal.IsInteger()) {
            throw RuntimeException("-- operation only can used on Integer " + ctx->ShortStack());
        }
        Value ret = oldVal;
        oldVal.Integer--;
        ctx->SetVarValue(name, oldVal);
        return ret;
    }

    case Instructions::kuURSHIFT: {
        if (!oldVal.IsInteger() || !val.IsInteger()) {
            throw RuntimeException(">>>= operation only can used on Integer " + ctx->ShortStack());
        }
        uint64_t i = ((uint64_t)oldVal.ToInteger() >> val.ToInteger());
        oldVal.Integer = (Value::INTVAR)i;
        break;
    }
    case Instructions::kuMOD: {
        oldVal %= val;
        break;
    }
    default:
        LOG_ERROR("Unknown Instruction:" + ToString((int64_t)opCode));
    }
    ctx->SetVarValue(name, oldVal);
    return oldVal;
}

Value Executor::ExecuteIfStatement(const Instruction* ins, VMContext* ctx) {
    const Instruction* one = GetInstruction(ins->Refs[0]);
    const Instruction* tow = GetInstruction(ins->Refs[1]);
    const Instruction* three = GetInstruction(ins->Refs[2]);
    Value val = Execute(one, ctx);
    if (val.ToBoolean()) {
        return Value();
    }
    if (ctx->IsExecutedInterupt()) {
        return Value();
    }
    if (tow->OpCode != Instructions::kNop) {
        std::vector<const Instruction*> branchs = GetInstructions(tow->Refs);
        std::vector<const Instruction*>::iterator iter = branchs.begin();
        while (iter != branchs.end()) {
            val = Execute(*iter, ctx);
            if (val.ToBoolean()) {
                break;
            }
            if (ctx->IsExecutedInterupt()) {
                return Value();
            }
            iter++;
        }
    }
    if (val.ToBoolean()) {
        return Value();
    }
    if (three->OpCode == Instructions::kNop) {
        return Value();
    }
    Execute(three, ctx);
    return Value();
}

Value Executor::ExecuteBinaryOperation(const Instruction* ins, VMContext* ctx) {
    if (ins->OpCode == Instructions::kADD) {
        return BatchAddOperation(ins, ctx);
    }
    const Instruction* first = GetInstruction(ins->Refs[0]);
    Value firstVal = Execute(first, ctx);
    if (ins->OpCode == Instructions::kNOT) {
        return Value(!firstVal.ToBoolean());
    }
    if (ins->OpCode == Instructions::kBNG) {
        return ~firstVal;
    }
    if (ins->OpCode == Instructions::kAND) {
        if (!firstVal.ToBoolean()) {
            return Value(false);
        }
    }
    if (ins->OpCode == Instructions::kOR) {
        if (firstVal.ToBoolean()) {
            return Value(true);
        }
    }
    const Instruction* second = GetInstruction(ins->Refs[1]);
    Value secondVal = Execute(second, ctx);
    /* if (secondVal.IsInteger() && firstVal.IsNULL()) {
        firstVal.Type = ValueType::kInteger;
        firstVal.Integer = 0;
    }*/

    switch (ins->OpCode) {
    case Instructions::kSUB:
        return firstVal - secondVal;
    case Instructions::kMUL:
        return firstVal * secondVal;
    case Instructions::kDIV:
        return firstVal / secondVal;
    case Instructions::kMOD:
        return firstVal % secondVal;
    case Instructions::kGT:
        return Value(firstVal > secondVal);
    case Instructions::kGE:
        return Value(firstVal >= secondVal);
    case Instructions::kLT:
        return Value(firstVal < secondVal);
    case Instructions::kLE:
        return Value(firstVal <= secondVal);
    case Instructions::kEQ:
        return Value(firstVal == secondVal);
    case Instructions::kNE:
        return Value(firstVal != secondVal);
    case Instructions::kBAND:
        return firstVal & secondVal;
    case Instructions::kBOR:
        return firstVal | secondVal;
    case Instructions::kBXOR:
        return firstVal ^ secondVal;
    case Instructions::kLSHIFT:
        return firstVal << secondVal;
    case Instructions::kRSHIFT:
        return firstVal >> secondVal;
    case Instructions::kOR:
        return Value(firstVal.ToBoolean() || secondVal.ToBoolean());
    case Instructions::kAND:
        return Value(firstVal.ToBoolean() && secondVal.ToBoolean());
    case Instructions::kURSHIFT: {
        if (!firstVal.IsInteger() || !secondVal.IsInteger()) {
            throw RuntimeException(">>> operation only can used on Integer " + ctx->ShortStack());
        }
        uint64_t i = ((uint64_t)firstVal.ToInteger() >> secondVal.ToInteger());
        return Value((int64_t)i);
    }
    default:
        LOG_ERROR("Unknow OpCode:" + ins->ToString());
        return Value();
    }
}

Value Executor::ExecuteList(std::vector<const Instruction*> insList, VMContext* ctx) {
    std::vector<const Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        Execute(*iter, ctx);
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    return Value();
}

Value Executor::GetFunction(const std::string& name, VMContext* ctx) {
    Value ret;
    if (ctx == NULL) {
        ctx = mCurrentVMContxt;
    }
    const Instruction* func = ctx->GetFunction(name);
    if (func != NULL) {
        return Value(func);
    }
    RUNTIME_FUNCTION method = GetBuiltinMethod(name);
    if (method == NULL) {
        return Value();
    }
    return Value(method);
}

Value Executor::CallFunction(const Instruction* ins, VMContext* ctx) {
    VMContext::ObjectCreator* creator = ctx->GetObjectCreator(ins->Name);
    if (creator != NULL) {
        return ExecuteNewObject(creator, GetInstruction(ins->Refs[0]), ctx);
    }
    Value func = GetFunction(ins->Name, ctx);
    //LOG("Call function:", ins->Name);
    if (func.Type == ValueType::kRuntimeFunction) {
        return CallRutimeFunction(ins, ctx, func.RuntimeFunction);
    } else if (func.Type == ValueType::kFunction) {
        return CallScriptFunction(ins, ctx, func.Function);
    } else {
        throw RuntimeException("can't as function called :" + ins->Name + " " + ctx->ShortStack());
    }
}

scoped_refptr<VMContext> Executor::FillActualParameters(const Instruction* func,
                                                        std::vector<Value>& values,
                                                        VMContext* ctx) {
    scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx, 0, func->Name);
    const Instruction* formal = NULL;
    std::vector<const Instruction*> formalList;
    if (func->Refs.size() == 2) {
        formal = GetInstruction(func->Refs[1]);
        formalList = GetInstructions(formal->Refs);
        for (auto iter : formalList) {
            Execute(iter, newCtx);
        }
    }
    newCtx->SetVarValue("_FCT_ANON_ARGS", Value(values), true);
    //函数不带参数
    if (formal == NULL) {
        return newCtx;
    }
    //可变参数
    if (formal->Name == KnownListName::kDeclMore) {
        Value list = Value(values);
        newCtx->SetVarValue(formalList.front()->Name, list);
        return newCtx;
    }
    size_t i = 0;
    for (i = 0; i < formalList.size(); i++) {
        if (i < values.size()) {
            newCtx->SetVarValue(formalList[i]->Name, values[i], true);
        } else {
            if (formalList[i]->Refs.size() == 0) {
                LOG_WARNING("parameters count incorrect ", ctx->ShortStack() + func->Name);
            }
        }
    }
    return newCtx;
}

scoped_refptr<VMContext> Executor::FillActualParameters(const Instruction* func,
                                                        const Instruction* actual, VMContext* ctx) {
    scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx, 0, func->Name);
    const Instruction* formal = NULL;
    std::vector<const Instruction*> formalList;
    auto actualList = GetInstructions(actual->Refs);
    if (func->Refs.size() == 2) {
        formal = GetInstruction(func->Refs[1]);
        formalList = GetInstructions(formal->Refs);
        for (auto iter : formalList) {
            Execute(iter, newCtx);
        }
    }
    //命名参数
    if (actual->Name == KnownListName::kNamedValue) {
        for (auto iter : actualList) {
            if (!IsNameInGroup(iter->Name, formalList)) {
                LOG_WARNING(iter->Name, " is not a parametr for " + func->Name, " ",
                            ctx->ShortStack());
            }
            Value val = Execute(GetInstruction(iter->Refs[0]), ctx);
            newCtx->SetVarValue(iter->Name, val, true);
        }
        return newCtx;
    }
    auto values = InstructionToValue(actualList, ctx);
    newCtx->SetVarValue("_FCT_ANON_ARGS", Value(values), true);
    //函数不带参数
    if (formal == NULL) {
        return newCtx;
    }
    //可变参数
    if (formal->Name == KnownListName::kDeclMore) {
        Value list = Value(values);
        newCtx->SetVarValue(formalList.front()->Name, list);
        return newCtx;
    }
    size_t i = 0;
    for (i = 0; i < formalList.size(); i++) {
        if (i < values.size()) {
            newCtx->SetVarValue(formalList[i]->Name, values[i], true);
        } else {
            if (formalList[i]->Refs.size() == 0) {
                LOG_WARNING("parameters count incorrect ", ctx->ShortStack() + func->Name);
            }
        }
    }
    return newCtx;
}

Value Executor::CallMethod(const Instruction* ins, VMContext* ctx) {
    Value where = Execute(GetInstruction(ins->Refs[0]), ctx);
    if (!where.IsObject()) {
        throw RuntimeException("can't call method on non object " + ctx->ShortStack());
    }
    std::string name = where.object->TypeName() + "#" + ins->Name;
    auto func = ctx->GetFunction(name);
    if (func == NULL) {
        throw RuntimeException("cant't find method:" + name + " " + ctx->ShortStack());
    }

    scoped_refptr<VMContext> newCtx = FillActualParameters(func, GetInstruction(ins->Refs[1]), ctx);
    newCtx->SetVarValue("self", where, true);

    mCurrentVMContxt = newCtx;
    Execute(GetInstruction(func->Refs[0]), newCtx);
    Value val = newCtx->GetReturnValue();
    mCurrentVMContxt = ctx;
    return val;
}

Value Executor::CallRutimeFunction(const Instruction* ins, VMContext* ctx,
                                   RUNTIME_FUNCTION method) {
    std::vector<Value> actualValues;
    if (ins->Refs.size() == 1) {
        bool exception = false;
        std::string error;
        const Instruction* ac = GetInstruction(ins->Refs[0]);
        std::vector<const Instruction*> acs = GetInstructions(ac->Refs);
        try {
            actualValues = InstructionToValue(GetInstructions(ac->Refs), ctx);
        } catch (RuntimeException exp) {
            error = exp.what();
            exception = true;
        }
        if (exception && ac->Refs.size() == 1 && ins->Name == "typeof" &&
            acs.front()->OpCode == Instructions::kReadVar) {
            std::string check = "variable not found :" + acs.front()->Name;
            if (error.find(check) != std::string::npos) {
                Value ret = GetFunction(ins->Name, ctx);
                if (ret.IsFunction()) {
                    return Value("function");
                }
                return Value("undef");
            }
        }
        if (exception) {
            throw RuntimeException(error);
        }
    }
    if (ctx->IsExecutedInterupt()) {
        return ctx->GetReturnValue();
    }
    Value val = method(actualValues, ctx, this);
    return val;
}

Value Executor::CallScriptFunction(const Instruction* ins, VMContext* ctx,
                                   const Instruction* func) {
    scoped_refptr<VMContext> newCtx = FillActualParameters(func, GetInstruction(ins->Refs[0]), ctx);
    mCurrentVMContxt = newCtx;
    Execute(GetInstruction(func->Refs[0]), newCtx);
    Value val = newCtx->GetReturnValue();
    mCurrentVMContxt = ctx;
    return val;
}

Value Executor::CallScriptFunction(const std::string& name, std::vector<Value>& args,
                                   VMContext* ctx) {
    auto func = ctx->GetFunction(name);
    if (func == NULL) {
        return Value();
    }
    const Instruction* body = GetInstruction(func->Refs[0]);
    scoped_refptr<VMContext> newCtx = FillActualParameters(func, args, ctx);
    mCurrentVMContxt = newCtx;
    Execute(body, newCtx);
    Value val = newCtx->GetReturnValue();
    mCurrentVMContxt = ctx;
    return val;
}

Value Executor::CallObjectMethod(Value object, Value func, std::vector<Value>& value,
                                 VMContext* ctx) {
    if (!object.IsObject()) {
        throw RuntimeException(object.ToDescription() + " not a object " + ctx->ShortStack());
    }
    if (!func.IsFunction()) {
        throw RuntimeException(func.ToDescription() + " not a callable object " +
                               ctx->ShortStack());
    }
    if (ctx == NULL) {
        ctx = mCurrentVMContxt;
    }
    scoped_refptr<VMContext> newCtx = FillActualParameters(func.Function, value, ctx);
    newCtx->SetVarValue("self", object, true);

    mCurrentVMContxt = newCtx;
    Execute(GetInstruction(func.Function->Refs[0]), newCtx);
    Value val = newCtx->GetReturnValue();
    mCurrentVMContxt = ctx;
    return val;
}

Value Executor::ExecuteForStatement(const Instruction* ins, VMContext* ctx) {
    const Instruction* init = GetInstruction(ins->Refs[0]);
    const Instruction* condition = GetInstruction(ins->Refs[1]);
    const Instruction* after = GetInstruction(ins->Refs[2]);
    const Instruction* block = GetInstruction(ins->Refs[3]);
    Execute(init, ctx);
    while (true) {
        Value val = Value(1l);
        if (!condition->IsNULL()) {
            val = Execute(condition, ctx);
        }
        if (!val.ToBoolean()) {
            break;
        }
        Execute(block, ctx);
        ctx->CleanContinueFlag();
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        Execute(after, ctx);
    }
    return Value();
}

//TODO 实现UserObject clone
Value Executor::ExecuteForInStatement(const Instruction* ins, VMContext* ctx) {
    const Instruction* iter_able_obj = GetInstruction(ins->Refs[0]);
    const Instruction* body = GetInstruction(ins->Refs[1]);
    std::list<std::string> key_val = split(ins->Name, ',');
    std::string key = key_val.front(), val = key_val.back();
    Value objVal = Execute(iter_able_obj, ctx);
    ctx->AddVar(val);
    if (key.size()) {
        ctx->AddVar(key);
    }
    switch (objVal.Type) {
    case ValueType::kString: {
        for (size_t i = 0; i < objVal.Length(); i++) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, Value((long)i));
            }
            ctx->SetVarValue(val, Value((long)objVal.text[i]));
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;
    case ValueType::kBytes: {
        for (size_t i = 0; i < objVal.Length(); i++) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, Value((long)i));
            }
            ctx->SetVarValue(val, Value((long)objVal.text[i]));
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;
    case ValueType::kArray: {
        for (size_t i = 0; i < objVal._array().size(); i++) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, Value((long)i));
            }
            ctx->SetVarValue(val, objVal._array()[i]);
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;
    case ValueType::kMap: {
        std::map<Value, Value>::iterator iter = objVal._map().begin();
        while (iter != objVal._map().end()) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, iter->first);
            }
            ctx->SetVarValue(val, iter->second);
            iter++;
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    } break;

    case ValueType::kObject: {
        auto groups = objVal.Object()->__enum_all();
        for (auto iter : groups) {
            if (key.size() > 0) {
                ctx->SetVarValue(key, iter.first);
            }
            ctx->SetVarValue(val, iter.second);
            Execute(body, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt()) {
                break;
            }
        }
    }

    default:
        break;
    }
    return Value();
}

Value Executor::ExecuteCreateMap(const Instruction* ins, VMContext* ctx) {
    const Instruction* list = GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::MakeMap();
    }
    std::vector<const Instruction*> items = GetInstructions(list->Refs);
    Value val = Value::MakeMap();
    std::vector<const Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        const Instruction* key = GetInstruction((*iter)->Refs[0]);
        const Instruction* value = GetInstruction((*iter)->Refs[1]);
        Value keyVal = Execute(key, ctx);
        Value valVal = Execute(value, ctx);
        if (ctx->IsExecutedInterupt()) {
            return Value();
        }
        val._map()[keyVal] = valVal;
        iter++;
    }
    return val;
}
Value Executor::ExecuteCreateArray(const Instruction* ins, VMContext* ctx) {
    const Instruction* list = GetInstruction(ins->Refs[0]);
    if (list->OpCode == Instructions::kNop) {
        return Value::MakeArray();
    }
    std::vector<const Instruction*> items = GetInstructions(list->Refs);
    Value val = Value::MakeArray();
    std::vector<const Instruction*>::iterator iter = items.begin();
    while (iter != items.end()) {
        val._array().push_back(Execute((*iter), ctx));
        if (ctx->IsExecutedInterupt()) {
            return Value();
        }
        iter++;
    }
    return val;
}
Value Executor::ExecuteSlice(const Instruction* ins, VMContext* ctx) {
    const Instruction* from = GetInstruction(ins->Refs[0]);
    const Instruction* to = GetInstruction(ins->Refs[1]);
    Value fromVal = Execute(from, ctx);
    Value toVal = Execute(to, ctx);
    Value opObj = ctx->GetVarValue(ins->Name);
    return opObj.Slice(fromVal, toVal);
}

Value Executor::UpdateValueAt(Value& toObject, const Value& index, const Value& val,
                              Instructions::Type opCode) {
    Value oldVal = toObject.GetValue(index);
    if (oldVal.IsNULL()) {
        oldVal.Type = ValueType::kInteger;
        if (val.IsString()) {
            oldVal.Type = val.Type;
        }
    }
    switch (opCode & 0xFF) {
    case Instructions::kuWrite:
        oldVal = val;
        break;
    case Instructions::kuADD:
        oldVal += val;
        break;
    case Instructions::kuSUB:
        oldVal -= val;
        break;
    case Instructions::kuDIV:
        oldVal *= val;
        break;
    case Instructions::kuMUL:
        oldVal /= val;
        break;
    case Instructions::kuBOR:
        oldVal |= val;
        break;
    case Instructions::kuBAND:
        oldVal &= val;
        break;
    case Instructions::kuBXOR:
        oldVal ^= val;
        break;
    case Instructions::kuLSHIFT:
        oldVal <<= val;
        break;
    case Instructions::kuRSHIFT:
        oldVal >>= val;
        break;
    case Instructions::kuINC:
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("++ operation only can used on Integer ");
        }
        oldVal.Integer++;
        break;
    case Instructions::kuDEC:
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("-- operation only can used on Integer ");
        }
        oldVal.Integer--;
        break;
    case Instructions::kuINCReturnOld: {
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("++ operation only can used on Integer ");
        }
        Value ret = oldVal;
        oldVal.Integer++;
        toObject.SetValue(index, oldVal);
        return ret;
    }

    case Instructions::kuDECReturnOld: {
        if (!oldVal.IsInteger()) {
            DUMP_CONTEXT();
            throw RuntimeException("-- operation only can used on Integer ");
        }
        Value ret = oldVal;
        oldVal.Integer--;
        toObject.SetValue(index, oldVal);
        return ret;
    }

    case Instructions::kuURSHIFT: {
        if (!oldVal.IsInteger() || !val.IsInteger()) {
            throw RuntimeException(">>>= operation only can used on Integer ");
        }
        uint64_t i = ((uint64_t)oldVal.ToInteger() >> val.ToInteger());
        oldVal.Integer = (Value::INTVAR)i;
        break;
    }
    case Instructions::kuMOD: {
        oldVal %= val;
        break;
    }
    default:
        LOG_ERROR("Unknown Instruction code :" + ToString((int64_t)opCode));
    }
    toObject.SetValue(index, oldVal);
    return oldVal;
}

Value Executor::ConvertNil(Value index, VMContext* ctx) {
    auto func = ctx->GetFunction("__index_nil__");
    if (func != NULL) {
        std::vector<Value> args;
        args.push_back(index);
        return CallScriptFunction("__index_nil__", args, ctx);
    }
    if (index.IsInteger() && index.Integer < 4096) {
        Value ret = Value::MakeArray();
        ret._array().resize(index.IsInteger() + 1);
        return ret;
    }
    if (index.IsInteger()) {
        DUMP_CONTEXT();
        LOG_WARNING(
                "wanring please check auto convert nil to map use a integer key larger than "
                "4096",
                ctx->ShortStack());
    }
    return Value::MakeMap();
}

// Instruction* CreateReference(const std::string& root,Instruction* path);
// Instruction* VarUpdateExpression(Instruction* ref, Instruction* value, int opcode);
Value Executor::ExecuteUpdateObjectVar(const Instruction* ins, VMContext* ctx) {
    Value val;
    const Instruction* ref = GetInstruction(ins->Refs[0]);
    if (ins->Refs.size() == 2) {
        val = Execute(GetInstruction(ins->Refs[1]), ctx);
    }
    if (ref->Refs.size() == 0) {
        return UpdateVar(ref->Name, val, ins->OpCode, ctx);
    }
    std::vector<Value> indexer = ObjectPathToIndexer(GetInstruction(ref->Refs.front()), ctx);
    Value root;
    ctx->GetVarValue(ref->Name, root);
    if (root.IsNULL()) {
        if (indexer.size() > 1) {
            throw RuntimeException("index on nil object not incorrect " + ctx->ShortStack());
        }
        root = ConvertNil(indexer.front(), ctx);
        ctx->SetVarValue(ref->Name, root);
    }
    Value toObject = root;
    for (size_t i = 0; i < indexer.size() - 1; i++) {
        toObject = toObject.GetValue(indexer[i]);
    }
    val = UpdateValueAt(toObject, indexer.back(), val, ins->OpCode);
    if (root.IsString()) {
        ctx->SetVarValue(ref->Name, root);
    }
    return val;
}
// name ,indexer
Value Executor::ExecuteReadObjectVar(const Instruction* ins, VMContext* ctx) {
    const Instruction* index = GetInstruction(ins->Refs[0]);
    std::vector<Value> indexValues = ObjectPathToIndexer(index, ctx);
    Value root = ctx->GetVarValue(ins->Name);
    Value& toObject = root;
    for (size_t i = 0; i < indexValues.size() - 1; i++) {
        if (toObject.IsNULL()) {
            return Value();
        }
        toObject = toObject.GetValue(indexValues[i]);
    }
    if (toObject.IsNULL()) {
        return Value();
    }
    const Value& last = toObject;
    return last[indexValues.back()];
}

std::vector<Value> Executor::InstructionToValue(std::vector<const Instruction*> insList,
                                                VMContext* ctx) {
    std::vector<Value> result;
    std::vector<const Instruction*>::iterator iter = insList.begin();
    while (iter != insList.end()) {
        result.push_back(Execute(*iter, ctx));
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    return result;
}

Value Executor::ExecuteSwitchStatement(const Instruction* ins, VMContext* ctx) {
    const Instruction* value = GetInstruction(ins->Refs[0]);
    const Instruction* cases = GetInstruction(ins->Refs[1]);
    const Instruction* defaultBranch = GetInstruction(ins->Refs[2]);
    std::vector<const Instruction*> case_array = GetInstructions(cases->Refs);
    Value val = Execute(value, ctx);
    std::vector<const Instruction*>::iterator iter = case_array.begin();
    bool casehit = false;
    while (iter != case_array.end()) {
        std::vector<const Instruction*> conditions =
                GetInstructions(GetInstruction((*iter)->Refs[0])->Refs);
        std::vector<Value> condition_values = InstructionToValue(conditions, ctx);
        const Instruction* actions = GetInstruction((*iter)->Refs[1]);
        std::vector<Value>::iterator iter2 = condition_values.begin();
        bool found = false;
        while (iter2 != condition_values.end()) {
            if (val == *iter2) {
                found = true;
                break;
            }
            iter2++;
        }
        if (!found) {
            iter++;
            continue;
        }
        casehit = true;
        Execute(actions, ctx);
        if (ctx->IsExecutedInterupt()) {
            break;
        }
        iter++;
    }
    if (casehit) {
        return Value();
    }
    Execute(defaultBranch, ctx);
    return Value();
}

Value Executor::BatchAddOperation(const Instruction* ins, VMContext* ctx) {
    const Instruction* seek = ins;
    std::list<Value> groups;
    groups.push_back(Execute(GetInstruction(seek->Refs[1]), ctx));
    while (true) {
        const Instruction* ref0 = GetInstruction(seek->Refs[0]);
        if (ref0->OpCode != Instructions::kADD) {
            Value result = Execute(ref0, ctx);
            for (auto iter : groups) {
                result = result + iter;
            }
            return result;
        }
        seek = ref0;
        groups.push_front(Execute(GetInstruction(seek->Refs[1]), ctx));
    }
}

Value Executor::ExecuteNewObject(VMContext::ObjectCreator* creator, const Instruction* actual,
                                 VMContext* ctx) {
    if (!creator->Initialzed) {
        auto methodList = GetInstructions(creator->MethodsList->Refs);
        for (auto iter : methodList) {
            ctx->AddMethod(creator->Name, iter->Name, iter);
        }
        creator->Initialzed = true;
    }
    std::map<std::string, Value> attributes;
    for (auto iter : creator->Attributes) {
        attributes[iter.first] = iter.second.Clone();
    }

    if (!actual->IsNULL()) {
        if (actual->OpCode == Instructions::kGroup && actual->Name == KnownListName::kNamedValue &&
            actual->Refs.size() == creator->Attributes.size()) {
            auto list = GetInstructions(actual->Refs);
            for (auto iter : list) {
                Value val = Execute(iter, ctx);
                attributes[iter->Name] = val;
            }
        } else {
            std::stringstream o;
            o << "actual:" << actual->ToString() << " Refs.size=" << actual->Refs.size();
            LOG_DEBUG(o.str());
            DUMP_CONTEXT();
            throw RuntimeException("initialize " + creator->Name +
                                   " object paramters not invalid " + ctx->ShortStack());
        }
    }
    scoped_refptr<UDObject> obj = new UDObject(this, creator->Name, attributes);
    return Value::MakeObject(obj);
}

//    Instruction* ObjectDeclarationExpresion(const std::string& name, Instruction* varList,
//                                            Instruction* methods);
Value Executor::ExecuteObjectDeclaration(const Instruction* ins, VMContext* ctx) {
    auto varList = GetInstructions(GetInstruction(ins->Refs[0])->Refs);
    std::map<std::string, Value> attributes;
    for (auto iter : varList) {
        assert(iter->OpCode == Instructions::kNewVar);
        Value val;
        if (iter->Refs.size() == 1) {
            val = Execute(iter, ctx);
        }
        attributes[iter->Name] = val;
    }
    VMContext::ObjectCreator* creator = new VMContext::ObjectCreator();
    creator->Attributes = attributes;
    creator->MethodsList = GetInstruction(ins->Refs[1]);
    creator->Name = ins->Name;
    creator->Initialzed = false;
    ctx->AddObjectCreator(ins->Name, creator);
    return Value();
}

void InitializeLibray() {
#ifdef _DEBUG_SCRIPT
    VMContext::sTlsIndex = TLS::Allocate();
#endif
}

} // namespace Interpreter