#include "parser.hpp"

#include <sstream>

#include "logger.hpp"

namespace Interpreter {

namespace KnownListName {
#ifndef _DEBUG
const char* kDecl = "a";
const char* kDeclAssign = "b";
const char* kElseIf = "c";
const char* kConst = "d";
const char* kDeclMore = "e";
const char* kDeclFuncArgs = "f";
const char* kValue = "g";
const char* kNamedValue = "h";
const char* kAssign = "j";
const char* kMapValue = "k";
const char* kCaseItem = "o";
const char* kCase = "p";
const char* kBlockStatement = "r";
const char* kStatement = "s";
const char* kObjMethod = "t";
const char* kPath = "u";
#else
const char* kDecl = "decl";
const char* kDeclAssign = "decl-assign";
const char* kElseIf = "else-if";
const char* kConst = "const";
const char* kDeclMore = "decl-more";
const char* kDeclFuncArgs = "decl-func-args";
const char* kValue = "values";
const char* kNamedValue = "named-values";
const char* kAssign = "indexer-assign";
const char* kMapValue = "map-value";
const char* kCaseItem = "case-item";
const char* kCase = "case-switch";
const char* kBlockStatement = "block-statement";
const char* kStatement = "statement";
const char* kObjMethod = "object_method";
const char* kPath = "index_path";
#endif
}; // namespace KnownListName

Instruction* Parser::NULLObject() {
    return mScript->NULLInstruction();
}

Instruction* Parser::CreateList(const std::string& typeName, Instruction* element) {
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(element, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::VarUpdateExpression(Instruction* ref, Instruction* value,
                                         Instructions::Type opcode) {
    Instruction* obj = mScript->NewInstruction(ref);
    if (value != NULL) {
        obj->Refs.push_back(value->key);
    }
    obj->OpCode = opcode | Instructions::kUpdate;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadExpresion(const std::string& name) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kReadVar;
    obj->Name = name;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateConst(const std::string& value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(int64_t value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(double value) {
    return mScript->NewConst(value);
}
Instruction* Parser::CreateConst(BYTE value) {
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateFunctionCall(const std::string& name, Instruction* actualParameters) {
    if(actualParameters == NULL){
        actualParameters = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(actualParameters);
    obj->OpCode = Instructions::kCallFunction;
    obj->Name = name;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateMinus(Instruction* val) {
    Instruction* obj = mScript->NewInstruction(val);
    obj->OpCode = Instructions::kMinus;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateBinaryOperation(Instruction* first, Instruction* second,
                                           Instructions::Type opcode) {
    if (opcode < Instructions::kADD || opcode > Instructions::kMAXBinaryOP) {
        throw RuntimeException("opcode not invalid");
        return NULL;
    }
    Instruction* obj = mScript->NewInstruction(first);
    if (second != NULL) {
        obj->Refs.push_back(second->key);
    }
    obj->OpCode = opcode;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateConditionExpresion(Instruction* condition, Instruction* action) {
    Instruction* obj = mScript->NewInstruction(condition, action);
    obj->OpCode = Instructions::kContitionExpression;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateReturnStatement(Instruction* value) {
    Instruction* obj = mScript->NewInstruction();
    if (value != NULL) {
        obj->Refs.push_back(value->key);
    }
    obj->OpCode = Instructions::kRETURNStatement;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateBreakStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kBREAKStatement;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateContinueStatement() {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kCONTINUEStatement;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::CreateMapItem(Instruction* key, Instruction* value) {
    Instruction* obj = mScript->NewInstruction(key, value);
    obj->OpCode = Instructions::kGroup;
    obj->Name = "map-item";
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
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
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreatePath(const std::string& element) {
    Instruction* el = CreateConst(element);
    return CreatePath(el);
}
Instruction* Parser::CreatePath(Instruction* element) {
    Instruction* obj = CreateList(KnownListName::kPath, element);
    obj->OpCode = Instructions::kPath;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::AppendToPath(Instruction* path, Instruction* element) {
    path->Refs.push_back(element->key);
    return path;
}
Instruction* Parser::AppendToPath(Instruction* path, const std::string& element) {
    Instruction* el = CreateConst(element);
    return AppendToPath(path, el);
}

Instruction* Parser::CreateReference(const std::string& root, Instruction* path) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kPathReference;
    obj->Name = root;
    if (path != NULL) {
        obj->Refs.push_back(path->key);
    }
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
Instruction* Parser::VarReadReference(const std::string& root, Instruction* path) {
    Instruction* obj = mScript->NewInstruction();
    obj->OpCode = Instructions::kReadReference;
    obj->Name = root;
    if (path != NULL) {
        obj->Refs.push_back(path->key);
    }
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::ObjectDeclarationExpresion(const std::string& name, Instruction* varList,
                                                Instruction* methods) {
    Instruction* obj = mScript->NewInstruction(varList, methods);
    obj->OpCode = Instructions::kObjectDecl;
    obj->Name = name;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}

Instruction* Parser::CreateObjectMethodCall(const std::string& var, const std::string& method,
                                            Instruction* actualParameters) {
    Instruction* where = VarReadExpresion(var);
    if(actualParameters== NULL){
        actualParameters = NULLObject();
    }
    Instruction* obj = mScript->NewInstruction(where, actualParameters);
    obj->OpCode = Instructions::kCallObjectMethod;
    obj->Name = method;
    if (mLogInstruction) {
        NVT_LOG_DEBUG(mScript->DumpInstruction(obj, ""));
    }
    return obj;
}
} // namespace Interpreter