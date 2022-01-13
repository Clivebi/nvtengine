#pragma once
#include <assert.h>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "exception.hpp"
#include "value.hpp"

namespace Interpreter {
namespace Instructions {
typedef unsigned short Type;
const Type kNop = 0;
#define CODE_BASE (0)
const Type kConst = CODE_BASE + 1;
const Type kNewVar = CODE_BASE + 2;
const Type kReadVar = CODE_BASE + 3;
const Type kNewFunction = CODE_BASE + 5;
const Type kCallFunction = CODE_BASE + 6;
const Type kPathReference = CODE_BASE + 8;
const Type kGroup = CODE_BASE + 9;
const Type kContitionExpression = CODE_BASE + 10;
const Type kIFStatement = CODE_BASE + 11;
const Type kRETURNStatement = CODE_BASE + 12;
const Type kFORStatement = CODE_BASE + 13;
const Type kCONTINUEStatement = CODE_BASE + 14;
const Type kBREAKStatement = CODE_BASE + 15;
const Type kCreateMap = CODE_BASE + 16;
const Type kCreateArray = CODE_BASE + 17;
const Type kSlice = CODE_BASE + 18;
const Type kForInStatement = CODE_BASE + 19;
const Type kSwitchCaseStatement = CODE_BASE + 20;
const Type kMinus = CODE_BASE + 21;
const Type kReadReference = CODE_BASE + 22;
const Type kObjectDecl = CODE_BASE + 23;
const Type kCallObjectMethod = CODE_BASE + 24;
const Type kPath = CODE_BASE + 25;

const Type kBinaryOP = 40;
const Type kADD = kBinaryOP + 1;
const Type kSUB = kBinaryOP + 2;
const Type kMUL = kBinaryOP + 3;
const Type kDIV = kBinaryOP + 4;
const Type kMOD = kBinaryOP + 5;
const Type kGT = kBinaryOP + 6;
const Type kGE = kBinaryOP + 7;
const Type kLT = kBinaryOP + 8;
const Type kLE = kBinaryOP + 9;
const Type kEQ = kBinaryOP + 10;
const Type kNE = kBinaryOP + 11;
const Type kNOT = kBinaryOP + 12;
const Type kBOR = kBinaryOP + 13;
const Type kBAND = kBinaryOP + 14;
const Type kBXOR = kBinaryOP + 15;
const Type kBNG = kBinaryOP + 16;
const Type kLSHIFT = kBinaryOP + 17;
const Type kRSHIFT = kBinaryOP + 18;
const Type kOR = kBinaryOP + 19;
const Type kAND = kBinaryOP + 20;
const Type kURSHIFT = kBinaryOP + 21;
const Type kMAXBinaryOP = kBinaryOP + 21;

const Type kUpdate = 0x0100;
const Type kuWrite = 0;
const Type kuADD = 1;
const Type kuSUB = 2;
const Type kuMUL = 3;
const Type kuDIV = 4;
const Type kuINC = 5;
const Type kuDEC = 6;
const Type kuINCReturnOld = 7;
const Type kuDECReturnOld = 8;
const Type kuBOR = 9;
const Type kuBAND = 10;
const Type kuBXOR = 11;
const Type kuLSHIFT = 12;
const Type kuRSHIFT = 13;
const Type kuURSHIFT = 14;
const Type kuMOD = 15;
}; // namespace Instructions

namespace KnownListName {
extern const char* kDecl;
extern const char* kDeclAssign;
extern const char* kElseIf;
extern const char* kConst;
extern const char* kDeclMore;
extern const char* kDeclFuncArgs;
extern const char* kValue;
extern const char* kNamedValue;
//extern const char* kIndexer;
extern const char* kAssign;
extern const char* kMapValue;
extern const char* kCaseItem;
extern const char* kCase;
extern const char* kBlockStatement;
extern const char* kStatement;
extern const char* kAddMulti;
extern const char* kObjMethod;
extern const char* kPath;
}; // namespace KnownListName

class Instruction {
public:
    typedef unsigned int keyType;
    Instructions::Type OpCode;
    keyType key;
    std::string Name;
    std::vector<keyType> Refs;

    void WriteToStream(std::ostream& o) const {
        o << OpCode;
        o << key;
        o << (unsigned char)Name.size();
        o.write(Name.c_str(), Name.size());
        o << (int)Refs.size();
        for (auto iter : Refs) {
            o << iter;
        }
    }

    void ReadFromStream(std::iostream& stream) {
        char name_size = 0;
        int count = 0;
        keyType value = 0;
        stream >> OpCode;
        stream >> key;
        stream >> name_size;
        char* buf = new char[name_size];
        stream.read(buf, name_size);
        Name.assign(buf, name_size);
        delete[] buf;
        stream >> count;
        for (int i = 0; i < count; i++) {
            stream >> value;
            Refs.push_back(value);
        }
    }

public:
    Instruction() : OpCode(Instructions::kNop), key(0) {}
    Instruction(Instruction* one) : key(0) { Refs.push_back(one->key); }
    Instruction(Instruction* one, Instruction* tow) : key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three) : key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
        Refs.push_back(three->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three, Instruction* four)
            : key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
        Refs.push_back(three->key);
        Refs.push_back(four->key);
    }
    bool IsNULL() const { return OpCode == Instructions::kNop; }
    std::string ToString() const {
        if (OpCode >= Instructions::kADD && OpCode <= Instructions::kMAXBinaryOP) {
            return "Binary Operation";
        }
        if ((OpCode & Instructions::kUpdate) == Instructions::kUpdate) {
            return "Update Var:" + Name;
        }
        switch (OpCode) {
        case Instructions::kNop:
            return "Nop";
        case Instructions::kConst:
            return "Create Const:";
        case Instructions::kNewVar:
            return "Create Var:" + Name;
        case Instructions::kReadVar:
            return "Read Var:" + Name;
        case Instructions::kNewFunction:
            return "Create Function:" + Name;
        case Instructions::kCallFunction:
            return "Call Function:" + Name;
        case Instructions::kGroup:
            return Name + "(list)";
        case Instructions::kContitionExpression:
            return "ContitionExpression";
        case Instructions::kIFStatement:
            return "if Statement";
        case Instructions::kRETURNStatement:
            return "return Statement";
        case Instructions::kFORStatement:
            return "for Statement";
        case Instructions::kBREAKStatement:
            return "break Statement";
        case Instructions::kCONTINUEStatement:
            return "continue Statement";
        case Instructions::kCreateMap:
            return "Create Map";
        case Instructions::kCreateArray:
            return "Create Array";
        case Instructions::kSlice:
            return "Slice Array";
        case Instructions::kForInStatement:
            return "for in statement";
        case Instructions::kSwitchCaseStatement:
            return "switch -- case statement";
        case Instructions::kPath:
            return "ref_path:" + Name;
        case Instructions::kObjectDecl:
            return "object define:" + Name;
        case Instructions::kCallObjectMethod:
            return "call object method:" + Name;
        case Instructions::kReadReference:
            return "read object value:" + Name;
        case Instructions::kPathReference:
            return "object path reference:" + Name;

        default:
            return "Unknown Op";
        }
    }
};

class Script : public CRefCountedThreadSafe<Script> {
public:
    Instruction* EntryPoint;
    std::string Name;
    explicit Script(std::string name) : Name(name) {
        EntryPoint = NULL;
        mInstructionKey = 1;
        mConstKey = 1;
        mInstructionTable[0] = new Instruction();
        mInstructionBase = 0;
        mConstBase = 0;
        Status::sScriptCount++;
    }
    ~Script() {
        for (std::map<Instruction::keyType, Instruction*>::iterator iter =
                     mInstructionTable.begin();
             iter != mInstructionTable.end(); iter++) {
            delete (iter->second);
        }
        mInstructionTable.clear();
        mConstTable.clear();
        Status::sScriptCount--;
    }

protected:
    Instruction::keyType mInstructionKey;
    Instruction::keyType mConstKey;
    Instruction::keyType mInstructionBase;
    Instruction::keyType mConstBase;
    std::map<Instruction::keyType, Instruction*> mInstructionTable;
    std::map<Instruction::keyType, Value> mConstTable;

public:
    bool IsRelocated() const { return mInstructionBase > 0 || mConstBase > 0; }
    void RelocateInstruction(Instruction::keyType newbase, Instruction::keyType newConstbase) {
        if (mInstructionBase != 0 || mConstBase != 0) {
            throw RuntimeException("script can only Relocate once");
        }
        for (std::map<Instruction::keyType, Instruction*>::iterator iter =
                     mInstructionTable.begin();
             iter != mInstructionTable.end(); iter++) {
            Instruction* ptr = iter->second;
            if (ptr->OpCode == Instructions::kConst) {
                assert(ptr->Refs[0] < mConstKey);
                ptr->Refs[0] = ptr->Refs[0] + newConstbase;
            } else {
                for (size_t i = 0; i < ptr->Refs.size(); i++) {
                    assert(ptr->Refs[i] < mInstructionKey);
                    ptr->Refs[i] = ptr->Refs[i] + newbase;
                }
            }
            ptr->key += newbase;
        }
        mInstructionBase = newbase;
        mConstBase = newConstbase;
    }

    bool IsContainInstruction(Instruction::keyType key) const {
        return key >= mInstructionBase && key < GetNextInstructionKey();
    }
    bool IsContainConst(Instruction::keyType key) const {
        return key >= mConstBase && key < GetNextConstKey();
    }
    Instruction::keyType GetNextInstructionKey() const {
        return mInstructionBase + mInstructionKey;
    }
    Instruction::keyType GetNextConstKey() const { return mConstBase + mConstKey; }

public:
    Instruction* NewGroup(Instruction* element) {
        Instruction* ins = new Instruction(element);
        ins->OpCode = Instructions::kGroup;
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* AddToGroup(Instruction* group, Instruction* element) {
        assert(group->OpCode == Instructions::kGroup);
        group->Refs.push_back(element->key);
        return group;
    }
    Instruction* NULLInstruction() { return mInstructionTable[0]; }
    Instruction* NewInstruction() {
        Instruction* ins = new Instruction();
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one) {
        Instruction* ins = new Instruction(one);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow) {
        Instruction* ins = new Instruction(one, tow);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow, Instruction* three) {
        Instruction* ins = new Instruction(one, tow, three);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    Instruction* NewInstruction(Instruction* one, Instruction* tow, Instruction* three,
                                Instruction* four) {
        Instruction* ins = new Instruction(one, tow, three, four);
        ins->key = mInstructionKey;
        mInstructionKey++;
        mInstructionTable[ins->key] = ins;
        return ins;
    }
    //TODO use const value pool
    Instruction* NewConst(const std::string& value) {
        Value val = Value(value);
        Instruction::keyType key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }
    Instruction* NewConst(int64_t value) {
        Value val = Value((Value::INTVAR)value);
        Instruction::keyType key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }
    Instruction* NewConst(double value) {
        Value val = Value(value);
        Instruction::keyType key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }
    Instruction* NewConst(BYTE value) {
        Value val = Value(value);
        Instruction::keyType key = mConstKey;
        mConstKey++;
        mConstTable[key] = val;
        Instruction* ins = NewInstruction();
        ins->OpCode = Instructions::kConst;
        ins->Refs.push_back(key);
        return ins;
    }

    Value GetConstValue(Instruction::keyType key) const {
        auto iter = mConstTable.find(key - mConstBase);
        if (iter != mConstTable.end()) {
            return iter->second;
        }
        throw RuntimeException("invalid const value key");
    }

    const Instruction* GetInstruction(Instruction::keyType key) const {
        auto iter = mInstructionTable.find(key - mInstructionBase);
        if (iter != mInstructionTable.end()) {
            return iter->second;
        }
        throw RuntimeException("invalid instruction key");
    }

    std::vector<const Instruction*> GetInstructions(std::vector<Instruction::keyType> keys) const {
        std::vector<const Instruction*> result;
        for (auto iter : keys) {
            result.push_back(GetInstruction(iter));
        }
        return result;
    }

    std::string DumpInstruction(const Instruction* ins, std::string prefix) const {
        std::stringstream stream;
        stream << prefix;
        if (ins->OpCode == Instructions::kConst) {
            stream << ins->key << " " << ins->ToString() << GetConstValue(ins->Refs[0]).ToString()
                   << std::endl;
            return stream.str();
        }
        stream << ins->key << " " << ins->ToString() << std::endl;
        if (ins->Refs.size() > 0) {
            std::vector<const Instruction*> subs = GetInstructions(ins->Refs);
            std::vector<const Instruction*>::iterator iter = subs.begin();
            while (iter != subs.end()) {
                stream << DumpInstruction(*iter, prefix + "\t");
                iter++;
            }
        }
        return stream.str();
    }
    void WriteToStream(std::ostream& o) const {
        o << EntryPoint->key;
        o << (long)mInstructionTable.size();
        o << (long)mConstTable.size();
        for (auto iter : mInstructionTable) {
            (iter.second)->WriteToStream(o);
        }
    }

    void ReadFromStream(std::iostream& stream) {}

protected:
    std::string JoinRefsName(const Instruction* ins) const {
        std::string refs = "";
        std::vector<const Instruction*> list = GetInstructions(ins->Refs);
        for (size_t i = 0; i < list.size(); i++) {
            refs += list[i]->Name;
            refs += ",";
        }
        if (refs.size() == 0) {
            return refs;
        }
        return refs.substr(0, refs.size() - 1);
    }
};
} // namespace Interpreter