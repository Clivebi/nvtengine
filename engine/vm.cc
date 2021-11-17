#include "vm.hpp"

#include "logger.hpp"

void RegisgerEngineBuiltinMethod(Interpreter::Executor *vm);

namespace Interpreter
{

    Executor::Executor(ExecutorCallback *callback) : mScriptList(), mCallback(callback)
    {
        RegisgerEngineBuiltinMethod(this);
    }

    bool Executor::Execute(scoped_refptr<Script> script, std::string &errmsg, bool showWarning)
    {
        bool bRet = false;
        mScriptList.push_back(script);
        scoped_refptr<VMContext> context = new VMContext(VMContext::File, NULL);
        context->SetEnableWarning(showWarning);
        try
        {
            Execute(script->EntryPoint, context);
            bRet = true;
        }
        catch (const RuntimeException &e)
        {
            errmsg = e.what();
        }
        mScriptList.clear();
        return bRet;
    }

    void Executor::RegisgerFunction(BuiltinMethod methods[], int count)
    {
        for (int i = 0; i < count; i++)
        {
            mBuiltinMethods[methods[i].name] = methods[i].func;
        }
    }

    RUNTIME_FUNCTION Executor::GetBuiltinMethod(const std::string &name)
    {
        std::map<std::string, RUNTIME_FUNCTION>::iterator iter = mBuiltinMethods.find(name);
        if (iter == mBuiltinMethods.end())
        {
            return NULL;
        }
        return iter->second;
    }

    void Executor::RequireScript(const std::string &name, VMContext *ctx)
    {
        std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
        while (iter != mScriptList.end())
        {
            if ((*iter)->Name == name)
            {
                return;
            }
            iter++;
        }

        if (mCallback)
        {
            scoped_refptr<Script> required = mCallback->LoadScript(name.c_str());
            if (required.get() == NULL)
            {
                throw RuntimeException("load script <" + name + "> failed");
            }
            scoped_refptr<Script> last = mScriptList.back();
            required->RelocateInstruction(last->GetNextInstructionKey() + 100,
                                          last->GetNextConstKey() + 100);
            mScriptList.push_back(required);
            Execute(required->EntryPoint, ctx);
        }
    }

    Value Executor::GetAvailableFunction(VMContext *ctx)
    {
        std::vector<Value> builtin;
        std::map<std::string, RUNTIME_FUNCTION>::iterator iter = mBuiltinMethods.begin();
        while (iter != mBuiltinMethods.end())
        {
            builtin.push_back(Value(iter->first));
            iter++;
        }
        Value result = Value::make_map();
        result._map()[Value("script")] = ctx->GetTotalFunction();
        result._map()[Value("builtin")] = builtin;
        return result;
    }

    const Instruction *Executor::GetInstruction(Instruction::keyType key)
    {
        std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
        while (iter != mScriptList.end())
        {
            if ((*iter)->IsContainInstruction(key))
            {
                return (*iter)->GetInstruction(key);
            }
            iter++;
        }
        char buf[16] = {0};
        snprintf(buf, 16, "%d", key);
        throw RuntimeException(std::string("unknown instruction key:") + buf);
    }

    std::vector<const Instruction *> Executor::GetInstructions(std::vector<Instruction::keyType> keys)
    {
        if (keys.size() == 0)
        {
            return std::vector<const Instruction *>();
        }
        std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
        while (iter != mScriptList.end())
        {
            if ((*iter)->IsContainInstruction(keys[0]))
            {
                return (*iter)->GetInstructions(keys);
            }
            iter++;
        }
        char buf[16] = {0};
        snprintf(buf, 16, "%d", keys[0]);
        throw RuntimeException(std::string("unknown instruction key:") + buf);
    }

    Value Executor::GetConstValue(Instruction::keyType key)
    {
        std::list<scoped_refptr<Script>>::iterator iter = mScriptList.begin();
        while (iter != mScriptList.end())
        {
            if ((*iter)->IsContainConst(key))
            {
                return (*iter)->GetConstValue(key);
            }
            iter++;
        }
        throw RuntimeException("unknown const key");
    }

    Value Executor::Execute(const Instruction *ins, VMContext *ctx)
    {
        //LOG("execute " + ins->ToString());
        if (ctx->IsExecutedInterupt())
        {
            LOG("Instruction execute interupted :" + ins->ToString());
            return ctx->GetReturnValue();
        }
        if (ins->OpCode >= Instructions::kADD && ins->OpCode <= Instructions::kMAXArithmeticOP)
        {
            return ExecuteArithmeticOperation(ins, ctx);
        }
        if (ins->OpCode >= Instructions::kWrite && ins->OpCode <= Instructions::kRSHIFTWrite)
        {
            return ExecuteUpdateVar(ins, ctx);
        }
        switch (ins->OpCode)
        {
        case Instructions::kNop:
            return Value();
        case Instructions::kConst:
            return GetConstValue(ins->Refs[0]);
        case Instructions::kNewVar:
        {
            ctx->AddVar(ins->Name);
            if (ins->Refs.size())
            {
                Value initValue = Execute(GetInstruction(ins->Refs[0]), ctx);
                ctx->SetVarValue(ins->Name, initValue);
                return initValue;
            }
            return Value();
        }
        case Instructions::kReadVar:
            return ctx->GetVarValue(ins->Name);
        case Instructions::kMinus:
        {
            Value val = Execute(GetInstruction(ins->Refs.front()), ctx);
            switch (val.Type)
            {
            case ValueType::kInteger:
                val.Integer = (-val.Integer);
                return val;
            case ValueType::kFloat:
                val.Float = (-val.Float);
                return val;
            default:
                throw RuntimeException("minus operation only can applay to integer or float");
            }
        }
        case Instructions::kNewFunction:
        {
            if (ins->Name.size())
            {
                ctx->AddFunction(ins);
                return Value();
            }
            return Value(ins);
        }
        case Instructions::kCallFunction:
        {
            return CallFunction(ins, ctx);
        }

        case Instructions::kGroup:
        {
            ExecuteList(GetInstructions(ins->Refs), ctx);
            return Value();
        }

        case Instructions::kIFStatement:
        {
            ExecuteIfStatement(ins, ctx);
            return Value();
        }

        //return indcate the action executed or not
        case Instructions::kContitionExpression:
        {
            Value val = Execute(GetInstruction(ins->Refs[0]), ctx);
            if (val.ToBoolean())
            {
                Execute(GetInstruction(ins->Refs[1]), ctx);
            }
            return val;
        }

        case Instructions::kRETURNStatement:
        {
            ctx->ReturnExecuted(Execute(GetInstruction(ins->Refs[0]), ctx));
            return Value();
        }

        case Instructions::kFORStatement:
        {
            scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx);
            ExecuteForStatement(ins, newCtx);
            return Value();
        }
        case Instructions::kForInStatement:
        {
            scoped_refptr<VMContext> newCtx = new VMContext(VMContext::For, ctx);
            ExecuteForInStatement(ins, newCtx);
            return Value();
        }
        case Instructions::kSwitchCaseStatement:
        {
            scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Switch, ctx);
            ExecuteSwitchStatement(ins, newCtx);
            return Value();
        }
        case Instructions::kBREAKStatement:
        {
            ctx->BreakExecuted();
            return Value();
        }
        case Instructions::kCONTINUEStatement:
        {
            ctx->ContinueExecuted();
            return Value();
        }
        case Instructions::kCreateMap:
            return ExecuteCreateMap(ins, ctx);
        case Instructions::kCreateArray:
            return ExecuteCreateArray(ins, ctx);
        case Instructions::kReadAt:
            return ExecuteReadAt(ins, ctx);
        case Instructions::kWriteAt:
            return ExecuteWriteAt(ins, ctx);
        case Instructions::kSlice:
            return ExecuteSlice(ins, ctx);
        default:
            LOG("Unknown Instruction:" + ins->ToString());
            return Value();
        }
    }

    Value Executor::ExecuteUpdateVar(const Instruction *ins, VMContext *ctx)
    {
        Value val;
        if (ins->Refs.size())
        {
            val = Execute(GetInstruction(ins->Refs[0]), ctx);
        }
        if (ins->OpCode == Instructions::kWrite)
        {
            ctx->SetVarValue(ins->Name, val);
            return Value();
        }
        Value oldVal = ctx->GetVarValue(ins->Name);
        switch (ins->OpCode)
        {
        case Instructions::kADDWrite:
            oldVal += val;
            break;
        case Instructions::kSUBWrite:
            oldVal -= val;
            break;
        case Instructions::kDIVWrite:
            oldVal *= val;
            break;
        case Instructions::kMULWrite:
            oldVal /= val;
            break;
        case Instructions::kBORWrite:
            oldVal |= val;
            break;
        case Instructions::kBANDWrite:
            oldVal &= val;
            break;
        case Instructions::kBXORWrite:
            oldVal ^= val;
            break;
        case Instructions::kLSHIFTWrite:
            oldVal <<= val;
            break;
        case Instructions::kRSHIFTWrite:
            oldVal >>= val;
            break;
        case Instructions::kINCWrite:
            if (oldVal.Type != ValueType::kInteger)
            {
                throw RuntimeException("++ operation only can used on Integer ");
            }
            oldVal.Integer++;
            break;
        case Instructions::kDECWrite:
            if (oldVal.Type != ValueType::kInteger)
            {
                throw RuntimeException("-- operation only can used on Integer ");
            }
            oldVal.Integer--;
            break;
        default:
            LOG("Unknown Instruction:" + ins->ToString());
        }
        ctx->SetVarValue(ins->Name, oldVal);
        return oldVal;
    }

    Value Executor::ExecuteIfStatement(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *one = GetInstruction(ins->Refs[0]);
        const Instruction *tow = GetInstruction(ins->Refs[1]);
        const Instruction *three = GetInstruction(ins->Refs[2]);
        Value val = Execute(one, ctx);
        if (val.ToBoolean())
        {
            return Value();
        }
        if (ctx->IsExecutedInterupt())
        {
            return Value();
        }
        if (tow->OpCode != Instructions::kNop)
        {
            std::vector<const Instruction *> branchs = GetInstructions(tow->Refs);
            std::vector<const Instruction *>::iterator iter = branchs.begin();
            while (iter != branchs.end())
            {
                val = Execute(*iter, ctx);
                if (val.ToBoolean())
                {
                    break;
                }
                if (ctx->IsExecutedInterupt())
                {
                    return Value();
                }
                iter++;
            }
        }
        if (val.ToBoolean())
        {
            return Value();
        }
        if (three->OpCode == Instructions::kNop)
        {
            return Value();
        }
        Execute(three, ctx);
        return Value();
    }

    Value Executor::ExecuteArithmeticOperation(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *first = GetInstruction(ins->Refs[0]);
        Value firstVal = Execute(first, ctx);
        if (ins->OpCode == Instructions::kNOT)
        {
            return Value(!firstVal.ToBoolean());
        }
        if (ins->OpCode == Instructions::kBNG)
        {
            return ~firstVal;
        }
        const Instruction *second = GetInstruction(ins->Refs[1]);
        Value secondVal = Execute(second, ctx);

        switch (ins->OpCode)
        {
        case Instructions::kADD:
            return firstVal + secondVal;
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
        default:
            LOG("Unknow OpCode:" + ins->ToString());
            return Value();
        }
    }

    Value Executor::ExecuteList(std::vector<const Instruction *> insList, VMContext *ctx)
    {
        std::vector<const Instruction *>::iterator iter = insList.begin();
        while (iter != insList.end())
        {
            Execute(*iter, ctx);
            if (ctx->IsExecutedInterupt())
            {
                break;
            }
            iter++;
        }
        return Value();
    }

    Value Executor::GetFunction(const std::string &name, VMContext *ctx)
    {
        Value ret;
        const Instruction *func = ctx->GetFunction(name);
        if (func != NULL)
        {
            return Value(func);
        }
        RUNTIME_FUNCTION method = GetBuiltinMethod(name);
        if (method == NULL)
        {
            throw RuntimeException("variable not found :" + name);
        }
        return Value(method);
    }

    Value Executor::CallFunction(const Instruction *ins, VMContext *ctx)
    {
        Value func = GetFunction(ins->Name, ctx);
        if (func.Type == ValueType::kRuntimeFunction)
        {
            return CallRutimeFunction(ins, ctx, func.RuntimeFunction);
        }
        else if (func.Type == ValueType::kFunction)
        {
            return CallScriptFunction(ins, ctx, func.Function);
        }
        else
        {
            throw RuntimeException("can't as function called :" + ins->Name);
        }
    }

    Value Executor::CallRutimeFunction(const Instruction *ins, VMContext *ctx,
                                       RUNTIME_FUNCTION method)
    {
        std::vector<Value> actualValues;
        if (ins->Refs.size() == 1)
        {
            actualValues = InstructionToValue(GetInstructions(GetInstruction(ins->Refs[0])->Refs), ctx);
        }
        if (ctx->IsExecutedInterupt())
        {
            return ctx->GetReturnValue();
        }
        Value val = method(actualValues, ctx, this);
        return val;
    }

    Value Executor::CallScriptFunction(const Instruction *ins, VMContext *ctx,
                                       const Instruction *func)
    {
        std::vector<Value> actualValues;
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx);
        if (ins->Refs.size() == 1)
        {
            actualValues = InstructionToValue(GetInstructions(GetInstruction(ins->Refs[0])->Refs), ctx);
        }
        if (ctx->IsExecutedInterupt())
        {
            return ctx->GetReturnValue();
        }
        if (func->Refs.size() == 2)
        {
            std::vector<const Instruction *> formalParamers =
                GetInstructions(GetInstruction(func->Refs[1])->Refs);
            if (formalParamers.size() != actualValues.size())
            {
                throw RuntimeException("actual parameters count not equal formal paramers for func:" +
                                       ins->Name);
            }
            std::vector<const Instruction *>::iterator iter = formalParamers.begin();
            int i = 0;
            while (iter != formalParamers.end())
            {
                Execute(*iter, newCtx);
                newCtx->SetVarValue((*iter)->Name, actualValues[i]);
                i++;
                iter++;
            }
        }
        Execute(GetInstruction(func->Refs[0]), newCtx);
        Value val = newCtx->GetReturnValue();
        return val;
    }

    Value Executor::CallScriptFunctionWithNamedParameter(const Instruction *ins, VMContext *ctx, const Instruction *func)
    {
        std::vector<Value> actualValues;
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx);
        std::vector<const Instruction *> actual = GetInstructions(GetInstruction(ins->Refs[0])->Refs);
        std::vector<const Instruction *> formalParamers = GetInstructions(GetInstruction(func->Refs[1])->Refs);
        for (auto iter = actual.begin(); iter != actual.end(); iter++)
        {
            bool found = false;
            for (auto iter2 = formalParamers.begin(); iter2 != formalParamers.end(); iter2++)
            {
                if ((*iter)->Name == (*iter2)->Name)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                throw RuntimeException((*iter)->Name + " is not a parametr for " + ins->Name);
            }
        }

        for (auto iter = formalParamers.begin(); iter != formalParamers.end(); iter++)
        {
            Execute(*iter, ctx);
        }

        for (auto iter = actual.begin(); iter != actual.end(); iter++)
        {
            Execute(*iter, ctx);
        }
        Execute(GetInstruction(func->Refs[0]), newCtx);
        Value val = newCtx->GetReturnValue();
        return val;
    }
    Value Executor::CallScriptFunction(const std::string &name, std::vector<Value> &args,
                                       VMContext *ctx)
    {
        scoped_refptr<VMContext> newCtx = new VMContext(VMContext::Function, ctx);
        const Instruction *func = ctx->GetFunction(name);
        const Instruction *body = GetInstruction(func->Refs[0]);
        if (func->Refs.size() == 2)
        {
            const Instruction *formalParamersList = GetInstruction(func->Refs[0]);
            if (args.size() != formalParamersList->Refs.size())
            {
                throw RuntimeException("actual parameters count not equal formal paramers");
            }
            std::vector<const Instruction *> formalParamers = GetInstructions(formalParamersList->Refs);
            std::vector<const Instruction *>::iterator iter = formalParamers.begin();
            int i = 0;
            while (iter != formalParamers.end())
            {
                Execute(*iter, ctx);
                newCtx->SetVarValue((*iter)->Name, args[i]);
                i++;
                iter++;
            }
        }
        Execute(body, newCtx);
        Value val = newCtx->GetReturnValue();
        return val;
    }

    Value Executor::ExecuteForStatement(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *init = GetInstruction(ins->Refs[0]);
        const Instruction *condition = GetInstruction(ins->Refs[1]);
        const Instruction *after = GetInstruction(ins->Refs[2]);
        const Instruction *block = GetInstruction(ins->Refs[3]);
        Execute(init, ctx);
        while (true)
        {
            Value val = Value(1l);
            if (!condition->IsNULL())
            {
                val = Execute(condition, ctx);
            }
            if (!val.ToBoolean())
            {
                break;
            }
            Execute(block, ctx);
            ctx->CleanContinueFlag();
            if (ctx->IsExecutedInterupt())
            {
                break;
            }
            Execute(after, ctx);
        }
        return Value();
    }

    Value Executor::ExecuteForInStatement(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *iter_able_obj = GetInstruction(ins->Refs[0]);
        const Instruction *body = GetInstruction(ins->Refs[1]);
        std::list<std::string> key_val = split(ins->Name, ',');
        std::string key = key_val.front(), val = key_val.back();
        Value objVal = Execute(iter_able_obj, ctx);
        switch (objVal.Type)
        {
        case ValueType::kString:
        {
            for (size_t i = 0; i < objVal.bytes.size(); i++)
            {
                if (key.size() > 0)
                {
                    ctx->SetVarValue(key, Value((long)i));
                }
                ctx->SetVarValue(val, Value((long)objVal.bytes[i]));
                Execute(body, ctx);
                ctx->CleanContinueFlag();
                if (ctx->IsExecutedInterupt())
                {
                    break;
                }
            }
        }
        break;
        case ValueType::kArray:
        {
            for (size_t i = 0; i < objVal._array().size(); i++)
            {
                if (key.size() > 0)
                {
                    ctx->SetVarValue(key, Value((long)i));
                }
                ctx->SetVarValue(val, objVal._array()[i]);
                Execute(body, ctx);
                ctx->CleanContinueFlag();
                if (ctx->IsExecutedInterupt())
                {
                    break;
                }
            }
        }
        break;
        case ValueType::kMap:
        {
            std::map<Value, Value>::iterator iter = objVal._map().begin();
            while (iter != objVal._map().end())
            {
                if (key.size() > 0)
                {
                    ctx->SetVarValue(key, iter->first);
                }
                ctx->SetVarValue(val, iter->second);
                iter++;
                Execute(body, ctx);
                ctx->CleanContinueFlag();
                if (ctx->IsExecutedInterupt())
                {
                    break;
                }
            }
        }
        break;

        default:
            break;
        }
        return Value();
    }

    Value Executor::ExecuteCreateMap(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *list = GetInstruction(ins->Refs[0]);
        if (list->OpCode == Instructions::kNop)
        {
            return Value::make_map();
        }
        std::vector<const Instruction *> items = GetInstructions(list->Refs);
        Value val = Value::make_map();
        std::vector<const Instruction *>::iterator iter = items.begin();
        while (iter != items.end())
        {
            const Instruction *key = GetInstruction((*iter)->Refs[0]);
            const Instruction *value = GetInstruction((*iter)->Refs[1]);
            Value keyVal = Execute(key, ctx);
            Value valVal = Execute(value, ctx);
            if (ctx->IsExecutedInterupt())
            {
                return Value();
            }
            val._map()[keyVal] = valVal;
            iter++;
        }
        return val;
    }
    Value Executor::ExecuteCreateArray(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *list = GetInstruction(ins->Refs[0]);
        if (list->OpCode == Instructions::kNop)
        {
            return Value::make_array();
        }
        std::vector<const Instruction *> items = GetInstructions(list->Refs);
        Value val = Value::make_array();
        std::vector<const Instruction *>::iterator iter = items.begin();
        while (iter != items.end())
        {
            val._array().push_back(Execute((*iter), ctx));
            if (ctx->IsExecutedInterupt())
            {
                return Value();
            }
            iter++;
        }
        return val;
    }
    Value Executor::ExecuteSlice(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *from = GetInstruction(ins->Refs[0]);
        const Instruction *to = GetInstruction(ins->Refs[1]);
        Value fromVal = Execute(from, ctx);
        Value toVal = Execute(to, ctx);
        Value opObj = ctx->GetVarValue(ins->Name);
        return opObj.Slice(fromVal, toVal);
    }
    Value Executor::ExecuteWriteAt(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *where = GetInstruction(ins->Refs[0]);
        Value toObject = ctx->GetVarValue(ins->Name);
        const Instruction *value = GetInstruction(ins->Refs[1]);
        Value elementValue = Execute(value, ctx);
        if (where->OpCode != Instructions::kGroup)
        {
            Value index = Execute(where, ctx);
            toObject.SetValue(index, elementValue);
            return Value();
        }
        std::vector<const Instruction *> indexInsList = GetInstructions(where->Refs);
        std::vector<Value> indexValues = InstructionToValue(indexInsList, ctx);
        for (size_t i = 0; i < indexValues.size() - 1; i++)
        {
            toObject = toObject[indexValues[i]];
        }
        toObject.SetValue(indexValues.back(), elementValue);
        return Value();
    }
    Value Executor::ExecuteReadAt(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *where = GetInstruction(ins->Refs[0]);
        Value index = Execute(where, ctx);
        if (ins->Refs.size() == 1)
        {
            Value fromObject = ctx->GetVarValue(ins->Name);
            return fromObject[index];
        }
        assert(ins->Name.size() == 0);
        Value fromObject = Execute(GetInstruction(ins->Refs[1]), ctx);
        return fromObject[index];
    }

    std::vector<Value> Executor::InstructionToValue(std::vector<const Instruction *> insList,
                                                    VMContext *ctx)
    {
        std::vector<Value> result;
        std::vector<const Instruction *>::iterator iter = insList.begin();
        while (iter != insList.end())
        {
            result.push_back(Execute(*iter, ctx));
            if (ctx->IsExecutedInterupt())
            {
                break;
            }
            iter++;
        }
        return result;
    }

    Value Executor::ExecuteSwitchStatement(const Instruction *ins, VMContext *ctx)
    {
        const Instruction *value = GetInstruction(ins->Refs[0]);
        const Instruction *cases = GetInstruction(ins->Refs[1]);
        const Instruction *defaultBranch = GetInstruction(ins->Refs[2]);
        std::vector<const Instruction *> case_array = GetInstructions(cases->Refs);
        Value val = Execute(value, ctx);
        std::vector<const Instruction *>::iterator iter = case_array.begin();
        bool casehit = false;
        while (iter != case_array.end())
        {
            std::vector<const Instruction *> conditions =
                GetInstructions(GetInstruction((*iter)->Refs[0])->Refs);
            std::vector<Value> condition_values = InstructionToValue(conditions, ctx);
            const Instruction *actions = GetInstruction((*iter)->Refs[1]);
            std::vector<Value>::iterator iter2 = condition_values.begin();
            bool found = false;
            while (iter2 != condition_values.end())
            {
                if (val == *iter2)
                {
                    found = true;
                    break;
                }
                iter2++;
            }
            if (!found)
            {
                iter++;
                continue;
            }
            casehit = true;
            Execute(actions, ctx);
            if (ctx->IsExecutedInterupt())
            {
                break;
            }
            iter++;
        }
        if (casehit)
        {
            return Value();
        }
        Execute(defaultBranch, ctx);
        return Value();
    }

} // namespace Interpreter