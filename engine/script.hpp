#pragma once
#include <assert.h>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "exception.hpp"
#include "value.hpp"
#include "varint.hpp"

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

inline void BinaryWrite(std::ostream& stream, unsigned short val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, unsigned int val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, unsigned char val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, long long val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, short val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, int val) {
    unsigned char buf[9] = {0};
    int count = varint::encode((long long)val, buf);
    if (count == -1) {
        throw RuntimeException("val too larger");
    }
    stream.write((char*)buf, count);
}

inline void BinaryWrite(std::ostream& stream, double val) {
    stream.write((const char*)&val, sizeof(val));
}

inline void BinaryWrite(std::ostream& stream, const std::string& val) {
    unsigned int Size = (unsigned int)val.size();
    BinaryWrite(stream, Size);
    stream.write(val.c_str(), Size);
}


template <typename T>
inline void BinaryRead(std::istream& stream, T& val) {
    long long result = 0;
    int count = varint::decode(stream, result);
    if (count == -1) {
        throw RuntimeException("decode varint failed");
    }
    val = (T)result;
}

template <>
inline void BinaryRead(std::istream& stream, double& val) {
    stream.read((char*)&val, sizeof(double));
}

template <>
inline void BinaryRead(std::istream& stream, std::string& val) {
    unsigned int Size = 0;
    BinaryRead(stream, Size);
    if (Size == 0) {
        val = "";
        return;
    }
    char* buffer = new char[Size];
    stream.read(buffer, Size);
    val.assign(buffer, Size);
    delete[] buffer;
}

class Instruction {
public:
    typedef unsigned int keyType;
    Instructions::Type OpCode;
    keyType key;
    std::string Name;
    std::vector<keyType> Refs;

    void WriteToStream(std::ostream& o) const {
        BinaryWrite(o, OpCode);
        BinaryWrite(o, key);
        BinaryWrite(o, Name);
        unsigned int RefsSize = (unsigned int)Refs.size();
        BinaryWrite(o, RefsSize);
        for (auto iter : Refs) {
            BinaryWrite(o, iter);
        }
    }

    void ReadFromStream(std::istream& i) {
        BinaryRead(i, OpCode);
        BinaryRead(i, key);
        BinaryRead(i, Name);
        unsigned int RefSize = 0;
        BinaryRead(i, RefSize);
        keyType cKey;
        Refs.clear();
        for (unsigned int n = 0; n < RefSize; n++) {
            BinaryRead(i, cKey);
            Refs.push_back(cKey);
        }
    }

public:
    Instruction() : OpCode(Instructions::kNop), key(0) {}
    Instruction(Instruction* one) : OpCode(Instructions::kNop), key(0) { Refs.push_back(one->key); }
    Instruction(Instruction* one, Instruction* tow) : OpCode(Instructions::kNop), key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three)
            : OpCode(Instructions::kNop), key(0) {
        Refs.push_back(one->key);
        Refs.push_back(tow->key);
        Refs.push_back(three->key);
    }
    Instruction(Instruction* one, Instruction* tow, Instruction* three, Instruction* four)
            : OpCode(Instructions::kNop), key(0) {
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
        unsigned int InsSize, ConstSize;
        InsSize = (unsigned int)mInstructionTable.size();
        ConstSize = (unsigned int)mConstTable.size();

        BinaryWrite(o, EntryPoint->key);
        BinaryWrite(o, Name);
        BinaryWrite(o, InsSize);
        BinaryWrite(o, ConstSize);
        for (auto iter : mInstructionTable) {
            (iter.second)->WriteToStream(o);
        }
        for (auto iter : mConstTable) {
            BinaryWrite(o, iter.first);
            WriteConst(o, iter.second);
        }
    }

    void ReadFromStream(std::istream& i) {
        unsigned int InsSize, ConstSize;
        Instruction::keyType cKey = 0, Entry = 0;
        BinaryRead(i, Entry);
        BinaryRead(i, Name);
        BinaryRead(i, InsSize);
        BinaryRead(i, ConstSize);

        mInstructionTable.clear();
        mConstTable.clear();
        for (unsigned int n = 0; n < InsSize; n++) {
            Instruction* ins = new Instruction();
            ins->ReadFromStream(i);
            mInstructionTable[ins->key] = ins;
        }

        for (unsigned int n = 0; n < ConstSize; n++) {
            BinaryRead(i, cKey);
            Value val = ReadConst(i);
            mConstTable[cKey] = val;
        }
        EntryPoint = mInstructionTable[Entry];
        mInstructionKey = (Instruction::keyType)mInstructionTable.size() + 1;
        mConstKey = (Instruction::keyType)mConstTable.size() + 1;
    }

    void WriteConst(std::ostream& o, const Value& val) const {
        assert(val.IsAtom());
        if (!val.IsAtom()) {
            return;
        }
        BinaryWrite(o, val.Type);
        switch (val.Type) {
        case ValueType::kByte:
            BinaryWrite(o, val.Byte);
            break;
        case ValueType::kFloat:
            BinaryWrite(o, val.Float);
            break;
        case ValueType::kInteger:
            BinaryWrite(o, val.Integer);
            break;
        case ValueType::kString:
            BinaryWrite(o, val.text);
            break;
        default:
            break;
        }
    }

    Value ReadConst(std::istream& i) {
        Value val;
        BinaryRead(i, val.Type);
        assert(val.IsAtom());
        if (!val.IsAtom()) {
            return Value();
        }
        switch (val.Type) {
        case ValueType::kByte:
            BinaryRead(i, val.Byte);
            break;
        case ValueType::kInteger:
            BinaryRead(i, val.Integer);
            break;
        case ValueType::kFloat:
            BinaryRead(i, val.Float);
            break;
        case ValueType::kString:
            BinaryRead(i, val.text);
            break;
        default:
            throw RuntimeException("not a atom const value type");
        }
        return val;
    }

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