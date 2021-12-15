#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "base.hpp"
#include "exception.hpp"
#include "logger.hpp"

namespace Interpreter {
class Status {
public:
    static long sResourceCount;
    static long sArrayCount;
    static long sMapCount;
    static long sParserCount;
    static long sScriptCount;
    static long sVMContextCount;
    static std::string ToString() {
        std::stringstream o;
        o << "Res:\t\t" << sResourceCount << "\n";
        o << "Array:\t\t" << sArrayCount << "\n";
        o << "Map:\t\t" << sMapCount << "\n";
        o << "Parser:\t\t" << sParserCount << "\n";
        o << "Script:\t\t" << sScriptCount << "\n";
        o << "Context:\t" << sVMContextCount << "\n";
        return o.str();
    }
};

std::list<std::string> split(const std::string& text, char split_char);

std::string HTMLEscape(const std::string& src);

std::string ToString(double val);

std::string ToString(int64_t val);

std::string ToString(int val);

std::string ToString(unsigned long val);

std::string ToString(long val);

std::string ToString(unsigned int val);

std::string HexEncode(const char* buf, int count, std::string prefix = "");

std::string DecodeJSONString(const std::string& src);

std::string EncodeJSONString(const std::string& src, bool escape);

bool IsSafeSet(bool html, BYTE c);

bool IsMatchString(std::string word, std::string pattern);

bool IsVisableString(const std::string& src);

std::string& replace_str(std::string& str, const std::string& to_replaced,
                         const std::string& newchars, int maxcount);

class Resource : public CRefCountedThreadSafe<Resource> {
public:
    explicit Resource() { Status::sResourceCount++; }
    virtual ~Resource() {
        Close();
        Status::sResourceCount--;
    }
    virtual void Close() {};
    virtual bool IsAvaliable() = 0;
    virtual std::string TypeName() = 0;
};

class FileResource : public Resource {
public:
    FILE* mFile;

public:
    explicit FileResource(FILE* f) : Resource(), mFile(f) {}
    ~FileResource() { Close(); }
    void Close() {
        if (mFile != NULL) {
            fclose(mFile);
            mFile = NULL;
        }
    }
    bool IsAvaliable() { return mFile != NULL; }
    std::string TypeName() { return "File"; }
};

namespace ValueType {
const int kNULL = 0;
const int kByte = 1;
const int kInteger = 2;
const int kFloat = 3;
const int kString = 4;
const int kBytes = 5;
const int kArray = 6;
const int kMap = 7;
const int kObject = 8;
const int kFunction = 9;
const int kRuntimeFunction = 10;
const int kResource = 11;

inline std::string ToString(int Type) {
    switch (Type) {
    case ValueType::kString:
        return "string";
    case ValueType::kInteger:
        return "integer";
    case ValueType::kNULL:
        return "nil";
    case ValueType::kFloat:
        return "float";
    case ValueType::kArray:
        return "array";
    case ValueType::kMap:
        return "map";
    case ValueType::kBytes:
        return "bytes";
    case ValueType::kResource:
        return "resource";
    case ValueType::kFunction:
        return "function";
    case ValueType::kRuntimeFunction:
        return "function";
    case ValueType::kByte:
        return "byte";
    default:
        return "Unknown";
    }
}
}; // namespace ValueType

class Value;

class Object : public CRefCountedThreadSafe<Object> {
public:
    virtual ~Object() {}
    virtual std::string ObjectType() const = 0;
    virtual std::string MapKey() const { return Interpreter::ToString((int64_t)this); }
    virtual std::string ToString() const = 0;
    virtual std::string ToDescription() const =0;
    virtual std::string ToJSONString() const = 0;
};

class ArrayObject : public Object {
public:
    std::vector<Value> _array;
    explicit ArrayObject() : _array() { Status::sArrayCount++; }
    ~ArrayObject() { Status::sArrayCount--; }

public:
    std::string ObjectType() const { return "array"; };
    std::string ToString() const;
    std::string ToDescription() const;
    std::string ToJSONString() const;
    ArrayObject* Clone();
    DISALLOW_COPY_AND_ASSIGN(ArrayObject);
};

struct cmp_key {
    bool operator()(const Value& k1, const Value& k2) const;
};
typedef std::map<Value, Value, cmp_key> MAPTYPE;
class MapObject : public Object {
public:
    MAPTYPE _map;
    explicit MapObject() : _map() { Status::sMapCount++; }
    ~MapObject() { Status::sMapCount--; }

public:
    MapObject* Clone();
    std::string ObjectType() const { return "map"; };
    std::string ToString() const;
    std::string ToDescription() const;
    std::string ToJSONString() const;
    DISALLOW_COPY_AND_ASSIGN(MapObject);
};

inline bool IsMap(Object* obj) {
    return obj->ObjectType() == "map";
}
inline bool IsArray(Object* obj) {
    return obj->ObjectType() == "array";
}

typedef scoped_refptr<Object> OBJECTPTR;
typedef scoped_refptr<Resource> RESOURCE;
class VMContext;
class Executor;
typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values, VMContext* ctx, Executor* vm);
class Instruction;
class Value {
public:
    typedef int64_t INTVAR;
    unsigned char Type;
    union {
        uint8_t Byte;
        INTVAR Integer;
        double Float;
        const Instruction* Function;
        RUNTIME_FUNCTION RuntimeFunction;
    };
    std::string bytes;
    RESOURCE resource;
    OBJECTPTR object;

public:
    Value();
    Value(char);
    Value(unsigned char);
    Value(bool val);
    Value(long val);
    Value(INTVAR val);
    Value(int val);
    Value(unsigned int val);
    Value(size_t val);
    Value(double val);
    Value(std::string val);
    Value(const char* str);
    Value(const Value& val);
    Value(Resource*);
    Value(const Instruction*);
    Value(RUNTIME_FUNCTION func);
    Value(const std::vector<Value>& val);
    Value& operator=(const Value& right);

    static Value make_array();
    static Value make_bytes(std::string val);
    static Value make_map();

public:
    bool IsNULL() const { return Type == ValueType::kNULL; }
    bool IsInteger() const { return Type == ValueType::kInteger || Type == ValueType::kByte; }
    bool IsNumber() const {
        return Type == ValueType::kInteger || Type == ValueType::kFloat || Type == ValueType::kByte;
    }
    bool IsStringOrBytes() const { return Type == ValueType::kString || Type == ValueType::kBytes; }
    bool IsSameType(const Value& right) const { return Type == right.Type; }
    bool IsObject() const {
        return Type == ValueType::kObject || Type == ValueType::kArray || Type == ValueType::kMap;
    }
    bool IsMap() const { return Type == ValueType::kMap; }
    bool IsArray() const { return Type == ValueType::kArray; }
    bool IsFunction() {
        return Type == ValueType::kFunction || Type == ValueType::kRuntimeFunction;
    }
    bool IsResource(std::string& name) {
        if (Type == ValueType::kResource) {
            name = resource->TypeName();
            return true;
        }
        return false;
    }

    Value& operator+=(const Value& right);
    Value& operator-=(const Value& right) {
        if (!IsNumber() || !right.IsNumber()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("-= must used on integer or float");
        }
        if (Type == ValueType::kFloat) {
            Float -= right.ToFloat();
            return *this;
        }
        Integer -= right.ToInteger();
        return *this;
    }
    Value& operator*=(const Value& right) {
        if (!IsNumber() || !right.IsNumber()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("*= must used on integer or float");
        }
        if (Type == ValueType::kFloat) {
            Float *= right.ToFloat();
            return *this;
        }
        Integer *= right.ToInteger();
        return *this;
    }
    Value& operator/=(const Value& right) {
        if (!IsNumber() || !right.IsNumber()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("/= must used on integer or float");
        }
        if (Type == ValueType::kFloat) {
            Float /= right.ToFloat();
            return *this;
        }
        Integer /= right.ToInteger();
        return *this;
    }
    Value& operator%=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("%= must used on integer");
        }
        Integer %= right.ToInteger();
        return *this;
    }

    Value& operator>>=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException(">>= must used on integer");
        }
        Integer >>= right.ToInteger();
        return *this;
    }
    Value& operator<<=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException(">>= must used on integer");
        }
        Integer <<= right.ToInteger();
        return *this;
    }
    Value& operator|=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("|= must used on integer");
        }
        Integer |= right.ToInteger();
        return *this;
    }
    Value& operator&=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("&= must used on integer");
        }
        Integer &= right.ToInteger();
        return *this;
    }
    Value& operator^=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("^= must used on integer");
        }
        Integer ^= right.ToInteger();
        return *this;
    }
    Value operator~() {
        if (!IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("~ must used on integer");
        }
        return Value(~Integer);
    }

    Value Clone() const;

    ArrayObject* Array() const { return (ArrayObject*)object.get(); }
    MapObject* Map() const { return (MapObject*)object.get(); }
    MAPTYPE& _map() { return ((MapObject*)object.get())->_map; }
    std::vector<Value>& _array() { return ((ArrayObject*)object.get())->_array; }

    std::string MapKey() const;
    //转成一个字符串
    std::string ToString() const;
    //描述信息
    std::string ToDescription() const;
    std::string ToJSONString(bool escape = true) const;
    double ToFloat() const;
    INTVAR ToInteger() const;

    size_t Length() const;
    const Value operator[](const Value& key) const;
    Value& operator[](const Value& key);
    std::string TypeName() const { return ValueType::ToString(Type); };
    bool ToBoolean() const;
    Value Slice(const Value& from, const Value& to) const;
    void SetValue(const Value& key, const Value& val);
};

Value operator+(const Value& left, const Value& right);
Value operator-(const Value& left, const Value& right);
Value operator*(const Value& left, const Value& right);
Value operator/(const Value& left, const Value& right);
Value operator%(const Value& left, const Value& right);

bool operator>(const Value& left, const Value& right);
bool operator>=(const Value& left, const Value& right);
bool operator<(const Value& left, const Value& right);
bool operator<=(const Value& left, const Value& right);
bool operator==(const Value& left, const Value& right);
bool operator!=(const Value& left, const Value& right);

Value operator>>(const Value& left, const Value& right);
Value operator<<(const Value& left, const Value& right);
Value operator|(const Value& left, const Value& right);
Value operator&(const Value& left, const Value& right);
Value operator^(const Value& left, const Value& right);
} // namespace Interpreter