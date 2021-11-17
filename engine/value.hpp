#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "base.hpp"
#include "exception.hpp"
#include "logger.hpp"

namespace Interpreter {

std::list<std::string> split(const std::string& text, char split_char);
std::string HTMLEscape(const std::string& src);
std::string ToString(double val);
std::string ToString(int64_t val);
std::string HexEncode(const char* buf, int count);

class Resource : public CRefCountedThreadSafe<Resource> {
public:
    virtual ~Resource() { Close(); }
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
    std::string TypeName() { return "FileResource"; }
};

namespace ValueType {
const int kNULL = 0;
const int kInteger = 1;
const int kFloat = 2;
const int kString = 3;
const int kBytes = 4;
const int kArray = 5;
const int kMap = 6;
const int kObject = 7;
const int kFunction = 8;
const int kRuntimeFunction = 9;
const int kResource = 10;

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
    virtual std::string ToJSONString() const = 0;
};

class ArrayObject : public Object {
public:
    std::vector<Value> _array;

public:
    std::string ObjectType() const { return "array"; };
    std::string ToString() const;
    std::string ToJSONString() const;
};

struct cmp_key {
    bool operator()(const Value& k1, const Value& k2) const;
};
typedef std::map<Value, Value, cmp_key> MAPTYPE;
class MapObject : public Object {
public:
    MAPTYPE _map;

public:
    std::string ObjectType() const { return "map"; };
    std::string ToString() const;
    std::string ToJSONString() const;
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
        INTVAR Integer;
        double Float;
        const Instruction * Function;
        RUNTIME_FUNCTION    RuntimeFunction;
    };
    std::string bytes;
    RESOURCE resource;
    OBJECTPTR object;

public:
    Value();
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
    Value(const Instruction* );
    Value(RUNTIME_FUNCTION func );
    Value(const std::vector<Value>& val);
    Value& operator=(const Value& right);

    static Value make_array();
    static Value make_bytes(std::string val);
    static Value make_map();

public:
    bool IsInteger() const { return Type == ValueType::kInteger; }
    bool IsNumber() const { return Type == ValueType::kInteger || Type == ValueType::kFloat; }
    bool IsStringOrBytes() const { return Type == ValueType::kString || Type == ValueType::kBytes; }
    bool IsSameType(const Value& right) const { return Type == right.Type; }
    bool IsObject() const {
        return Type == ValueType::kObject || Type == ValueType::kArray || Type == ValueType::kMap;
    }
    bool IsFunction() {
        return Type == ValueType::kFunction || Type == ValueType::kRuntimeFunction;
    }

    Value& operator+=(const Value& right);
    Value& operator-=(const Value& right) {
        if (!IsNumber() || !right.IsNumber()) {
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
            throw Interpreter::RuntimeException("%= must used on integer");
        }
        Integer %= right.ToInteger();
        return *this;
    }

    Value& operator>>=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            throw Interpreter::RuntimeException(">>= must used on integer");
        }
        Integer >>= right.ToInteger();
        return *this;
    }
    Value& operator<<=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            throw Interpreter::RuntimeException(">>= must used on integer");
        }
        Integer <<= right.ToInteger();
        return *this;
    }
    Value& operator|=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            throw Interpreter::RuntimeException("|= must used on integer");
        }
        Integer |= right.ToInteger();
        return *this;
    }
    Value& operator&=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            throw Interpreter::RuntimeException("&= must used on integer");
        }
        Integer &= right.ToInteger();
        return *this;
    }
    Value& operator^=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            throw Interpreter::RuntimeException("^= must used on integer");
        }
        Integer ^= right.ToInteger();
        return *this;
    }
    Value operator~() {
        if (!IsInteger()) {
            throw Interpreter::RuntimeException("~ must used on integer");
        }
        return Value(~Integer);
    }

    ArrayObject* Array() { return (ArrayObject*)object.get(); }
    MapObject* Map() { return (MapObject*)object.get(); }
    MAPTYPE& _map() { return ((MapObject*)object.get())->_map; }
    std::vector<Value>& _array() { return ((ArrayObject*)object.get())->_array; }

    std::string MapKey() const;
    std::string ToString() const;
    std::string ToJSONString() const;
    double ToFloat() const;
    INTVAR ToInteger() const;

    size_t Length();
    Value operator[](const Value& key);
    std::string TypeName() const { return ValueType::ToString(Type); };
    bool ToBoolean();
    Value Slice(const Value& from, const Value& to);
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