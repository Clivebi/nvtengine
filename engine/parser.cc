#include "parser.hpp"

#include <sstream>

#include "logger.hpp"

namespace Interpreter {

Instruction* Parser::NULLObject() {
    return mScript->NULLInstruction();
}

Instruction* Parser::CreateList(const std::string& typeName, Instruction* element) {
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(element, ""));
    }
    Instruction* obj = mScript->NewGroup(element);
    obj->Name = typeName;
    return obj;
}

Instruction* Parser::AddToList(Instruction* list, Instruction* element) {
    return mScript->AddToGroup(list, element);
}

Instruction* Parser::VarDeclarationExpresion(const std::string& name, Instruction* value) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kNewVar;
    obj->Name = name;
    if (value != NULL) {
        obj->Refs.push_back(value->key);
    }
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::VarUpdateExpression(const std::string& name, Instruction* value, int opcode) {
    Instruction* obj = mScript->NewInstruction();
    if (value != NULL) {
        obj->Refs.push_back(value->key);
    }
    obj->Name = name;
    obj->OpCode = opcode;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadExpresion(const std::string& name) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kReadVar;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateConst(const std::string& value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(long value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(double value) {
    return mScript->NewConst(value);
}

Instruction* Parser::CreateFunction(const std::string& name, Instruction* formalParameters,
                                    Instruction* body) {
    Instruction* obj = mScript->NewInstruction(body);
    if (formalParameters != NULL) {
        obj->Refs.push_back(formalParameters->key);
    }
    obj->OpCode = Instructions::kNewFunction;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateFunctionCall(const std::string& name, Instruction* actualParameters) {
    Instruction* obj = mScript->NewInstruction();
    if (actualParameters != NULL) {
        obj->Refs.push_back(actualParameters->key);
    }
    obj->OpCode = Instructions::kCallFunction;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateMinus(Instruction* val) {
    Instruction* obj = mScript->NewInstruction(val);
    obj->OpCode = Instructions::kMinus;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateArithmeticOperation(Instruction* first, Instruction* second,
                                               int opcode) {
    if (opcode < Instructions::kADD || opcode > Instructions::kMAXArithmeticOP) {
        throw RuntimeException("opcode not invalid");
        return NULL;
    }
    Instruction* obj = mScript->NewInstruction(first);
    if (second != NULL) {
        obj->Refs.push_back(second->key);
    }
    obj->OpCode = opcode;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateConditionExpresion(Instruction* condition, Instruction* action) {
    Instruction* obj = mScript->NewInstruction(condition, action);
    obj->OpCode = Instructions::kContitionExpression;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateIFStatement(Instruction* one, Instruction* tow, Instruction* three) {
    if (tow == NULL) {
        tow = NULLObject();
    }
    if (three == NULL) {
        three = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(one, tow, three);
    obj->OpCode = Instructions::kIFStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateReturnStatement(Instruction* value) {
    Instruction* obj = mScript->NewInstruction(value);
    obj->OpCode = Instructions::kRETURNStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateForStatement(Instruction* init, Instruction* condition, Instruction* op,
                                        Instruction* body) {
    if (init == NULL) {
        init = NULLObject();
        condition = NULLObject();
        op = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(init, condition, op);
    obj->Refs.push_back(body->key);
    obj->OpCode = Instructions::kFORStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateBreakStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kBREAKStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateContinueStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kCONTINUEStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateMapItem(Instruction* key, Instruction* value) {
    Instruction* obj = mScript->NewInstruction(key, value);
    obj->OpCode = Instructions::kGroup;
    obj->Name = "map-item";
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateMap(Instruction* list) {
    if (list == NULL) {
        list = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(list);
    obj->OpCode = Instructions::kCreateMap;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateArray(Instruction* list) {
    if (list == NULL) {
        list = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(list);
    obj->OpCode = Instructions::kCreateArray;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadAtExpression(const std::string& name, Instruction* where) {
    Instruction* obj = mScript->NewInstruction(where);
    obj->OpCode = Instructions::kReadAt;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadAtExpression(const std::string& name, const std::string& key) {
    Instruction* where = CreateConst(key);
    return VarReadAtExpression(name, where);
}
Instruction* Parser::VarReadAtExpression(Instruction* obj, const std::string& key) {
    Instruction* where = CreateConst(key);
    return VarReadAtExpression(obj, where);
}
Instruction* Parser::VarReadAtExpression(Instruction* fromObj, Instruction* where) {
    Instruction* obj = mScript->NewInstruction(where, fromObj);
    obj->OpCode = Instructions::kReadAt;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarUpdateAtExression(const std::string& name, Instruction* where,
                                          Instruction* value) {
    assert(where->OpCode == Instructions::kGroup);
    if (where->Refs.size() == 1) {
        where = (Instruction*)mScript->GetInstruction(where->Refs[0]);
    }
    Instruction* obj = mScript->NewInstruction(where, value);
    obj->OpCode = Instructions::kWriteAt;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarSlice(const std::string& name, Instruction* from, Instruction* to) {
    if (from == NULL) {
        from = NULLObject();
    }
    if (to == NULL) {
        to = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(from, to);
    obj->OpCode = Instructions::kSlice;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateForInStatement(const std::string& key, const std::string& val,
                                          Instruction* iterobj, Instruction* body) {
    std::string name = key;
    name += ",";
    name += val;
    Instruction* obj = mScript->NewInstruction(iterobj, body);
    obj->OpCode = Instructions::kForInStatement;
    obj->Name = name;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateSwitchCaseStatement(Instruction* value, Instruction* cases,
                                               Instruction* defbranch) {
    if (defbranch == NULL) {
        defbranch = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(value, cases, defbranch);
    obj->OpCode = Instructions::kSwitchCaseStatement;
    if (mLogInstruction) {
        LOG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
} // namespace Interpreter