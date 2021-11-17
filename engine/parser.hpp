#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>

#include "script.hpp"

namespace Interpreter {
class Parser : public CRefCountedThreadSafe<Parser> {
protected:
    scoped_refptr<Script> mScript;

    std::list<std::string*> mStringHolder;
    std::string mScanningString;

    bool mLogInstruction;

public:
    Parser() : mScript(NULL), mLogInstruction(0), mStringHolder(), mScanningString() {}
    ~Parser() { Finish(); }

    void Start(std::string name) { mScript = new Script(name); }
    scoped_refptr<Script> Finish() {
        scoped_refptr<Script> ret = mScript;
        mScript = NULL;
        mScanningString.clear();
        std::list<std::string*>::iterator iter = mStringHolder.begin();
        while (iter != mStringHolder.end()) {
            delete (*iter);
            iter++;
        }
        mStringHolder.clear();
        return ret;
    }
    void SetLogInstruction(bool log) { mLogInstruction = log; }
    //string parser helper function
    void StartScanningString() { mScanningString.clear(); }
    void AppendToScanningString(char ch) { mScanningString += ch; }
    void AppendToScanningString(const char* text) { mScanningString += text; }
    const char* FinishScanningString() { return CreateString(mScanningString.c_str()); }
    const char* CreateString(const char* text) {
        //TODO: use string pools
        std::string* obj = new std::string(text);
        mStringHolder.push_back(obj);
        return obj->c_str();
    }

    //null instruction do nothing
    Instruction* NULLObject();
    //instruction list
    Instruction* CreateList(const std::string& typeName, Instruction* element);
    Instruction* AddToList(Instruction* list, Instruction* element);

    //var declaration read & write
    Instruction* VarDeclarationExpresion(const std::string& name, Instruction* value);
    Instruction* VarUpdateExpression(const std::string& name, Instruction* value, int opcode);
    Instruction* VarReadExpresion(const std::string& name);

    //const values
    Instruction* CreateConst(const std::string& value);
    Instruction* CreateConst(long value);
    Instruction* CreateConst(double value);

    //function define & call function
    Instruction* CreateFunction(const std::string& name, Instruction* formalParameters,
                                Instruction* body);
    Instruction* CreateFunctionCall(const std::string& name, Instruction* actualParameters);

    //arithmetic operation +-*/% ,> >= < <= == !=
    Instruction* CreateArithmeticOperation(Instruction* first, Instruction* second, int opcode);

    Instruction* CreateMinus(Instruction* value);

    //condition expresion such as if else if
    Instruction* CreateConditionExpresion(Instruction* condition, Instruction* action);

    //if(...){} else if(...){} else{} statement
    Instruction* CreateIFStatement(Instruction* one, Instruction* tow, Instruction* three);

    //return statement
    Instruction* CreateReturnStatement(Instruction* value);

    //for statement and break statement
    Instruction* CreateForStatement(Instruction* init, Instruction* condition, Instruction* op,
                                    Instruction* body);
    Instruction* CreateBreakStatement();
    Instruction* CreateContinueStatement();

    Instruction* CreateMapItem(Instruction* key, Instruction* value);
    Instruction* CreateMap(Instruction* list);
    Instruction* CreateArray(Instruction* list);
    Instruction* VarReadAtExpression(const std::string& name, Instruction* where);
    Instruction* VarReadAtExpression(const std::string& name, const std::string& where);
    Instruction* VarReadAtExpression(Instruction* obj, Instruction* where);
    Instruction* VarReadAtExpression(Instruction* obj, const std::string& where);
    Instruction* VarUpdateAtExression(const std::string& name, Instruction* where,
                                      Instruction* value);
    Instruction* VarSlice(const std::string& name, Instruction* from, Instruction* to);
    Instruction* CreateForInStatement(const std::string& key, const std::string& val,
                                      Instruction* obj, Instruction* body);
    Instruction* CreateSwitchCaseStatement(Instruction* value, Instruction* cases,
                                           Instruction* defbranch);
    //parser ending
    void SetEntryPoint(Instruction* value) { mScript->EntryPoint = value; }
};

}; // namespace Interpreter
