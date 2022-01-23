#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    static long sUDObjectCount;
    static std::string ToString() {
        std::stringstream o;
        o << "Res:\t\t" << sResourceCount << "\n";
        o << "Array:\t\t" << sArrayCount << "\n";
        o << "Map:\t\t" << sMapCount << "\n";
        o << "Parser:\t\t" << sParserCount << "\n";
        o << "Script:\t\t" << sScriptCount << "\n";
        o << "Context:\t" << sVMContextCount << "\n";
        o << "UDObj:\t\t" << sUDObjectCount << "\n";
        return o.str();
    }
};

std::list<std::string> split(const std::string& text, char split_char);

std::string ToString(double val);

std::string ToString(long long val);

std::string ToString(int val);

std::string ToString(unsigned long val);

std::string ToString(long val);

std::string ToString(unsigned int val);

std::string HexEncode(const char* buf, size_t count, std::string prefix = "");

std::string DecodeJSONString(const std::string& src);

std::string EncodeJSONString(const std::string& src, bool escape);

bool IsSafeSet(bool html, BYTE c);

bool IsMatchString(std::string word, std::string pattern);

bool IsVisableString(const std::string& src);

bool IsDigestString(const std::string& src);

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
    virtual std::string TypeName() const = 0;
    virtual std::string MapKey() const { return Interpreter::ToString((int64_t)this); }
    virtual std::string ToString() const = 0;
    virtual std::string ToDescription() const = 0;
    virtual std::string ToJSONString() const = 0;
};

class ArrayObject : public Object {
public:
    std::vector<Value> _array;
    explicit ArrayObject() : _array() { Status::sArrayCount++; }
    ~ArrayObject() { Status::sArrayCount--; }

public:
    std::string TypeName() const { return "array"; };
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
    std::string TypeName() const { return "map"; };
    std::string ToString() const;
    std::string ToDescription() const;
    std::string ToJSONString() const;
    DISALLOW_COPY_AND_ASSIGN(MapObject);
};

class VMContext;
class Executor;

class UDObject : public Object {
protected:
    Executor* mEngine;
    const std::string mTypeName;
    std::map<std::string, Value> mAttributes;
    DISALLOW_COPY_AND_ASSIGN(UDObject);

public:
    explicit UDObject(Executor* Engine, const std::string& Type,
                      std::map<std::string, Value>& attributes);
    ~UDObject();

    std::string TypeName() const { return mTypeName; };

    std::string ToString() const;

    std::string ToDescription() const;

    std::string ToJSONString() const;

    UDObject* Clone() const;

    //meta method
    size_t __length() const;
    Value __get_attr(Value key) const;
    Value __set_attr(Value key, Value val);
    std::list<std::pair<Value, Value>> __enum_all() const;
    bool __equal(const UDObject* other) const;
};

typedef scoped_refptr<Object> OBJECTPTR;
typedef scoped_refptr<Resource> RESOURCE;
typedef Value (*RUNTIME_FUNCTION)(std::vector<Value>& values, VMContext* ctx, Executor* vm);

class VMContext;
class Executor;
class Instruction;

//bytes type backend
class Bytes : public CRefCountedThreadSafe<Bytes> {
protected:
    friend class BytesView;
    BYTE* mData;
    size_t mLen;
    DISALLOW_COPY_AND_ASSIGN(Bytes);

public:
    explicit Bytes(size_t size) {
        mLen = size;
        mData = new BYTE[mLen];
        memset(mData, 0, mLen);
    }
    ~Bytes() { delete[] mData; }
};

class BytesView {
protected:
    scoped_refptr<Bytes> mBackend;
    size_t mViewStart;
    size_t mViewSize;

protected:
    const BYTE* GetDataPtr() const { return mBackend->mData + mViewStart; }

public:
    BytesView() : mBackend(NULL), mViewStart(0), mViewSize(0) {}
    BytesView(size_t Len) {
        mBackend = new Bytes(Len);
        mViewStart = 0;
        mViewSize = Len;
    }
    BytesView(const BytesView& view) {
        mBackend = view.mBackend;
        mViewStart = view.mViewStart;
        mViewSize = view.mViewSize;
    }
    BytesView& operator=(const BytesView& view) {
        mBackend = view.mBackend;
        mViewStart = view.mViewStart;
        mViewSize = view.mViewSize;
        return *this;
    }

    size_t Length() const { return mViewSize; }

    BytesView Slice(size_t left, size_t right) const {
        if (right == 0 || right > mViewSize) {
            right = mViewSize;
        }
        if (left > mViewSize) {
            throw RuntimeException("Bytes slice out of range");
        }
        BytesView view(*this);
        view.mViewStart = mViewStart + left;
        view.mViewSize = right - left;
        return view;
    }

    BytesView operator+(const BytesView& right) const {
        size_t Total = Length() + right.Length();
        BytesView ret(Total);
        memcpy(ret.mBackend->mData, GetDataPtr(), Length());
        memcpy(ret.mBackend->mData + Length(), right.GetDataPtr(), right.Length());
        return ret;
    }

    BYTE GetAt(size_t index) const {
        if (index >= Length()) {
            throw RuntimeException("Bytes GetAt index out of range");
        }
        return GetDataPtr()[index];
    }

    void CopyFrom(const void* from, size_t size) {
        BYTE* ptr = mBackend->mData + mViewStart;
        if (size > Length()) {
            size = Length();
        }
        memcpy(ptr, from, size);
    }

    void CopyFrom(std::string& src) { return CopyFrom(src.c_str(), src.size()); }

    void SetAt(size_t index, int Val) {
        if (index >= Length()) {
            throw RuntimeException("Bytes GetAt index out of range");
        }
        mBackend->mData[mViewStart + index] = (BYTE)(Val & 0xFF);
    }

    std::string ToString() const {
        std::string ret = "";
        ret.assign((char*)GetDataPtr(), Length());
        return ret;
    }
    std::string ToDescription() const {
        return "bytes(" + HexEncode((char*)GetDataPtr(), Length()) + ")";
    }
    std::string ToJSONString() const { return EncodeJSONString(ToString(), true); }
};

class Value {
public:
    typedef long long INTVAR;
    unsigned char Type;
    union {
        uint8_t Byte;
        INTVAR Integer;
        double Float;
        const Instruction* Function;
        RUNTIME_FUNCTION RuntimeFunction;
    };
    BytesView bytesView;
    std::string text;
    RESOURCE resource;
    OBJECTPTR object;

    void Reset() {
        object = NULL;
        resource = NULL;
        text = "";
        bytesView = BytesView();
        Function = 0;
        Integer = 0;
        Type = ValueType::kNULL;
    }

public:
    Value();
    Value(char);
    Value(unsigned char);
    Value(bool val);
    Value(long val);
    Value(INTVAR val);
    Value(int val);
    Value(unsigned int val);
#ifndef WIN_386
    Value(size_t val);
#endif
    Value(double val);
    Value(std::string val);
    Value(const char* str);
    Value(const Value& val);
    Value(Resource*);
    Value(const Instruction*);
    Value(RUNTIME_FUNCTION func);
    Value(const std::vector<Value>& val);
    Value& operator=(const Value& right);

    static Value MakeArray();
    static Value MakeBytes(size_t size);
    static Value MakeBytes(std::string src);
    static Value MakeMap();
    static Value MakeObject(UDObject* obj);

public:
    bool IsNULL() const { return Type == ValueType::kNULL; }
    bool IsInteger() const { return Type == ValueType::kInteger || Type == ValueType::kByte; }
    bool IsNumber() const {
        return Type == ValueType::kInteger || Type == ValueType::kFloat || Type == ValueType::kByte;
    }
    bool IsByte() const { return Type == ValueType::kByte; }
    bool IsString() const { return Type == ValueType::kString; }
    bool IsBytes() const { return Type == ValueType::kBytes; }
    bool IsSameType(const Value& right) const { return Type == right.Type; }
    bool IsObject() const { return Type == ValueType::kObject; }
    bool IsMap() const { return Type == ValueType::kMap; }
    bool IsArray() const { return Type == ValueType::kArray; }
    bool IsFunction() {
        return Type == ValueType::kFunction || Type == ValueType::kRuntimeFunction;
    }
    bool IsResource(std::string& name) const {
        if (Type == ValueType::kResource) {
            name = resource->TypeName();
            return true;
        }
        return false;
    }

    Value& operator+=(const Value& right);
    Value& operator-=(const Value& right) {
        if (IsString() && right.IsString()) {
            std::string result = text;
            text = replace_str(result, right.text, "", 1);
            return *this;
        }
        if (!IsNumber() || !right.IsNumber()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("-= must used on integer or float " +
                                                ToDescription() + "," + right.ToDescription());
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
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("*= must used on integer or float " +
                                                ToDescription() + "," + right.ToDescription());
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
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("/= must used on integer or float " +
                                                ToDescription() + "," + right.ToDescription());
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
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("%= must used on integer " + ToDescription() + "," +
                                                right.ToDescription());
        }
        Integer %= right.ToInteger();
        return *this;
    }

    Value& operator>>=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException(">>= must used on integer " + ToDescription() +
                                                "," + right.ToDescription());
        }
        Integer >>= right.ToInteger();
        return *this;
    }
    Value& operator<<=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException(">>= must used on integer " + ToDescription() +
                                                "," + right.ToDescription());
        }
        Integer <<= right.ToInteger();
        return *this;
    }
    Value& operator|=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("|= must used on integer " + ToDescription() + "," +
                                                right.ToDescription());
        }
        Integer |= right.ToInteger();
        return *this;
    }
    Value& operator&=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("&= must used on integer " + ToDescription() + "," +
                                                right.ToDescription());
        }
        Integer &= right.ToInteger();
        return *this;
    }
    Value& operator^=(const Value& right) {
        if (!IsInteger() || !right.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("^= must used on integer " + ToDescription() + "," +
                                                right.ToDescription());
        }
        Integer ^= right.ToInteger();
        return *this;
    }
    Value operator~() {
        if (!IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("~ must used on integer " + ToDescription());
        }
        return Value(~Integer);
    }

    //access underlayer data struct
    ArrayObject* Array() const { return (ArrayObject*)object.get(); }
    MapObject* Map() const { return (MapObject*)object.get(); }
    MAPTYPE& _map() { return ((MapObject*)object.get())->_map; }
    std::vector<Value>& _array() { return ((ArrayObject*)object.get())->_array; }
    UDObject* Object() const {
        assert(IsObject());
        return (UDObject*)(object.get());
    }
    //new element in the map or array
    Value& operator[](const Value& key);
    //common method
    //create a new copy of this value
    Value Clone() const;
    //retrun the key as the map key
    std::string MapKey() const;
    //convert to a string
    std::string ToString() const;
    //return the description
    std::string ToDescription() const;
    //convert to json string
    std::string ToJSONString(bool escape = true) const;
    //convert to float
    double ToFloat() const;
    //convert to integer
    INTVAR ToInteger() const;
    //get the length
    size_t Length() const;
    //get the value at the index(key)
    const Value operator[](const Value& key) const;
    Value GetValue(const Value& key) const;
    //set the value at the index(key)
    void SetValue(const Value& key, const Value& val);
    //get the type value type name
    std::string TypeName() const;
    //convet to boolean
    bool ToBoolean() const;
    //make a slice work on bytes or array
    Value Slice(const Value& from, const Value& to) const;
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