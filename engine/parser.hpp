#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>

#include "script.hpp"

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif

namespace Interpreter {
class Parser : public CRefCountedThreadSafe<Parser> {
protected:
    scoped_refptr<Script> mScript;

    std::list<std::string*> mStringHolder;
    std::string mScanningString;

    bool mLogInstruction;

    void* mUserContext;

public:
    std::string mLastError;
    Parser(void* context) : mScript(NULL), mLogInstruction(0), mStringHolder(), mScanningString() {
        Status::sParserCount++;
        mUserContext = context;
    }
    ~Parser() {
        Finish();
        Status::sParserCount--;
    }

    void* GetContext() { return mUserContext; }

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

    //
    //Instruction* CreateObjectIndexer(const std::string& first);
    //Instruction* AddObjectIndexer(Instruction* src, const std::string& item);

    Instruction* CreatePath(const std::string &element);
    Instruction* CreatePath(Instruction* element);
    Instruction* AppendToPath(Instruction* path,Instruction* element);
    Instruction* AppendToPath(Instruction* path,const std::string &element);

    Instruction* CreateReference(const std::string& root, Instruction* path);
    Instruction* VarReadReference(const std::string& root, Instruction* path);

    //var declaration read & write
    Instruction* VarDeclarationExpresion(const std::string& name, Instruction* value);
    Instruction* VarUpdateExpression(Instruction* ref, Instruction* value,
                                     Instructions::Type opcode);
    Instruction* VarReadExpresion(const std::string& name);

    //const values
    Instruction* CreateConst(const std::string& value);
    Instruction* CreateConst(int64_t value);
    Instruction* CreateConst(double value);
    Instruction* CreateConst(BYTE value);

    //function define & call function
    Instruction* CreateFunction(const std::string& name, Instruction* formalParameters,
                                Instruction* body);
    Instruction* CreateFunctionCall(const std::string& name, Instruction* actualParameters);

    //arithmetic operation +-*/% ,> >= < <= == !=
    Instruction* CreateBinaryOperation(Instruction* first, Instruction* second,
                                       Instructions::Type opcode);

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
    //while statement 
    Instruction* CreateWhileStatement(Instruction* condition,Instruction* block);
    Instruction* CreateDoWhileStatement(Instruction* block,Instruction* condition);
    Instruction* CreateBreakStatement();
    Instruction* CreateContinueStatement();

    Instruction* CreateMapItem(Instruction* key, Instruction* value);
    Instruction* CreateMap(Instruction* list);
    Instruction* CreateArray(Instruction* list);

    Instruction* VarSlice(const std::string& name, Instruction* from, Instruction* to);
    Instruction* CreateForInStatement(const std::string& key, const std::string& val,
                                      Instruction* obj, Instruction* body);
    Instruction* CreateSwitchCaseStatement(Instruction* value, Instruction* cases,
                                           Instruction* defbranch);
    Instruction* ObjectDeclarationExpresion(const std::string& name, Instruction* varList,
                                            Instruction* methods);
    Instruction* CreateObjectMethodCall(const std::string& var, const std::string& method,
                                        Instruction* actualParameters);
    //parser ending
    void SetEntryPoint(Instruction* value) { mScript->EntryPoint = value; }
};

}; // namespace Interpreter
