#include "value.hpp"

#include <string.h>

#include <sstream>

#include "utf8.hpp"

namespace Interpreter {
long Status::sResourceCount = 0;
long Status::sArrayCount = 0;
long Status::sMapCount = 0;
long Status::sParserCount = 0;
long Status::sScriptCount = 0;
long Status::sVMContextCount = 0;

std::list<std::string> split(const std::string& text, char split_char) {
    std::list<std::string> result;
    std::string part = "";
    std::string::const_iterator iter = text.begin();
    while (iter != text.end()) {
        if (*iter == split_char) {
            result.push_back(part);
            part = "";
            iter++;
            continue;
        }
        part += (*iter);
        iter++;
    }
    if (part.size()) {
        result.push_back(part);
    }
    return result;
}

static std::string safeSet = " !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{}|~\x7F";

bool IsSafeSet(bool html, BYTE c) {
    if (c >= 'a' && c <= 'z') {
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        return true;
    }
    if (c >= '0' && c <= '9') {
        return true;
    }
    if (std::string::npos == safeSet.find((char)c)) {
        return false;
    }
    if (c == '\"' || c == '\\') {
        return false;
    }
    if (html && (c == '&' || c == '<' || c == '>')) {
        return false;
    }
    return true;
}

void JSONEncodeString(std::stringstream& o, const std::string& src, bool escape) {
    const char* hex = "0123456789abcdef";
    size_t start = 0;
    for (size_t i = 0; i < src.size();) {
        BYTE b = src[i];
        if (b < 0x80 || !escape) {
            if (IsSafeSet(escape, b)) {
                i++;
                continue;
            }
            if (start < i) {
                o << src.substr(start, i - start);
            }
            o << "\\";
            switch (b) {
            case '\\':
            case '\"':
                o << (char)b;
                break;
            case '\r':
                o << "r";
                break;
            case '\t':
                o << "t";
                break;
            case '\n':
                o << "n";
                break;

            default:
                o << "u00";
                o << hex[b >> 4];
                o << hex[b & 0xF];
            }
            i++;
            start = i;
            continue;
        }
        int d_size;
        int32_t c = utf8::DecodeRune(src.substr(i), d_size);
        if (c == utf8::RuneError && d_size == 1) {
            if (start < i) {
                o << src.substr(start, i - start);
            }
            o << "\\fffd";
            i += d_size;
            start = i;
            continue;
        }
        o << "\\u";
        o << utf8::RuneString(c);
        i += d_size;
        start = i;
    }
    if (start < src.size()) {
        o << src.substr(start);
    }
}

std::string EncodeJSONString(const std::string& src, bool escape) {
    std::stringstream o;
    JSONEncodeString(o, src, escape);
    return o.str();
}

std::string DecodeJSONString(const std::string& src) {
    std::stringstream o;
    size_t start = 0;
    for (size_t i = 0; i < src.size();) {
        if (src[i] != '\\') {
            i++;
            continue;
        }
        if (start < i) {
            o << src.substr(start, i - start);
        }
        if (i + 1 >= src.size()) {
            return src;
        }
        i++;
        switch (src[i]) {
        case 'r':
            o << "\r";
            i++;
            break;
        case 'n':
            o << "\n";
            i++;
            break;
        case 't':
            o << "\t";
            i++;
            break;
        case '\"':
            o << "\"";
            i++;
            break;
        case 'u':
            i++;
            if (i + 4 > src.size()) {
                return src;
            }
            o << utf8::EncodeRune(utf8::ParseHex4((BYTE*)src.c_str() + i));
            i += 4;
            break;
        default:
            i++;
            o << src[i];
        }
        start = i;
    }
    if (start < src.size()) {
        o << src.substr(start);
    }
    return o.str();
}

std::string ToString(double val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%f", val);
    return buffer;
}
std::string ToString(int val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%d", val);
    return buffer;
}

std::string ToString(unsigned long val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%ld", val);
    return buffer;
}

std::string ToString(long val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%ld", val);
    return buffer;
}

std::string ToString(unsigned int val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%d", val);
    return buffer;
}

std::string ToString(int64_t val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%lld", val);
    return buffer;
}

std::string HexEncode(const char* buf, int count) {
    char buffer[6] = {0};
    std::string result = "";
    for (size_t i = 0; i < count; i++) {
        snprintf(buffer, 6, "%02X", (unsigned char)buf[i]);
        result += buffer;
    }
    return result;
}

std::string ArrayObject::ToJSONString() const {
    std::stringstream o;
    bool first = true;
    o << "[";
    auto iter = _array.begin();
    while (iter != _array.end()) {
        if (!first) {
            o << ",";
        }
        o << (*iter).ToJSONString();
        first = false;
        iter++;
    }
    o << "]";
    return o.str();
}

// Recursive function to check if the input string matches
// with a given wildcard pattern
bool IsMatchString(std::string word, int n, std::string pattern, int m) {
    // end of the pattern is reached
    if (m == pattern.size()) {
        // return true only if the end of the input string is also reached
        return n == word.size();
    }

    // if the input string reaches its end, return when the
    // remaining characters in the pattern are all '*'
    if (n == word.size()) {
        for (int i = m; i < pattern.size(); i++) {
            if (pattern[i] != '*') {
                return false;
            }
        }

        return true;
    }

    // if the current wildcard character is '?' or the current character in
    // the pattern is the same as the current character in the input string
    if (pattern[m] == '?' || pattern[m] == word[n]) {
        // move to the next character in the pattern and the input string
        return IsMatchString(word, n + 1, pattern, m + 1);
    }

    // if the current wildcard character is '*'
    if (pattern[m] == '*') {
        // move to the next character in the input string or
        // ignore '*' and move to the next character in the pattern
        return IsMatchString(word, n + 1, pattern, m) || IsMatchString(word, n, pattern, m + 1);
    }

    // we reach here when the current character in the pattern is not a
    // wildcard character, and it doesn't match the current
    // character in the input string
    return false;
}

// Check if a string matches with a given wildcard pattern
bool IsMatchString(std::string word, std::string pattern) {
    return IsMatchString(word, 0, pattern, 0);
}

bool cmp_key::operator()(const Value& k1, const Value& k2) const {
    return k1.MapKey() < k2.MapKey();
}
std::string MapObject::ToJSONString() const {
    std::stringstream o;
    bool first = true;
    o << "{";
    auto iter = _map.begin();
    while (iter != _map.end()) {
        //TODO check invalid json key
        if (!first) {
            o << ",";
        }
        o << "\"";
        o << iter->first.ToString();
        o << "\"";
        o << ":";
        o << iter->second.ToJSONString();
        first = false;
        iter++;
    }
    o << "}";
    return o.str();
}
std::string ArrayObject::ToString() const {
    std::stringstream o;
    bool first = true;
    o << "[";
    auto iter = _array.begin();
    while (iter != _array.end()) {
        if (!first) {
            o << ",";
        }
        o << (*iter).ToString();
        first = false;
        iter++;
    }
    o << "]";
    return o.str();
}
std::string MapObject::ToString() const {
    std::stringstream o;
    bool first = true;
    o << "{";
    auto iter = _map.begin();
    while (iter != _map.end()) {
        if (!first) {
            o << ",";
        }
        o << iter->first.MapKey();
        o << ":";
        o << iter->second.ToString();
        first = false;
        iter++;
    }
    o << "}";
    return o.str();
}

Value::Value(char val) : Type(ValueType::kByte), Byte(val), bytes(), resource(NULL), object(NULL) {
    Integer = 0;
    Byte = val;
}
Value::Value(unsigned char val)
        : Type(ValueType::kByte), Byte(val), bytes(), resource(NULL), object(NULL) {
    Integer = 0;
    Byte = val;
}

Value::Value() : Type(ValueType::kNULL), Integer(0), bytes(), resource(NULL), object(NULL) {}

Value::Value(bool val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(long val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(int val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(INTVAR val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(unsigned int val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(size_t val)
        : Type(ValueType::kInteger), Integer(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(double val)
        : Type(ValueType::kFloat), Float(val), bytes(), resource(NULL), object(NULL) {}
Value::Value(const char* str)
        : Type(ValueType::kString), Integer(0), bytes(str), resource(NULL), object(NULL) {}
Value::Value(std::string val)
        : Type(ValueType::kString), Integer(0), bytes(val), resource(NULL), object(NULL) {}

Value::Value(const Instruction* f)
        : Type(ValueType::kFunction), Function(f), bytes(), resource(NULL), object(NULL) {}
Value::Value(RUNTIME_FUNCTION f)
        : Type(ValueType::kRuntimeFunction),
          RuntimeFunction(f),
          bytes(),
          resource(NULL),
          object(NULL) {}

Value::Value(const Value& val) {
    switch (val.Type) {
    case ValueType::kBytes:
    case ValueType::kString:
        Integer = 0;
        bytes = val.bytes;
        break;
    case ValueType::kResource:
        Integer = 0;
        resource = val.resource;
        break;
    case ValueType::kInteger:
    case ValueType::kByte:
        Integer = val.Integer;
        break;
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        object = val.object;
        Integer = 0;
        break;
    case ValueType::kFloat:
        Float = val.Float;
        break;
    case ValueType::kFunction:
        Function = val.Function;
        break;
    case ValueType::kRuntimeFunction:
        RuntimeFunction = val.RuntimeFunction;
        break;
    case ValueType::kNULL:
        Integer = 0;
        break;
    default:
        throw RuntimeException("unknown value type!!!");
    }
    Type = val.Type;
}
Value& Value::operator=(const Value& val) {
    switch (val.Type) {
    case ValueType::kBytes:
    case ValueType::kString:
        Integer = 0;
        bytes = val.bytes;
        break;
    case ValueType::kResource:
        Integer = 0;
        resource = val.resource;
        break;
    case ValueType::kInteger:
    case ValueType::kByte:
        Integer = val.Integer;
        break;
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        object = val.object;
        Integer = 0;
        break;
    case ValueType::kFloat:
        Float = val.Float;
        break;
    case ValueType::kFunction:
        Function = val.Function;
        break;
    case ValueType::kRuntimeFunction:
        RuntimeFunction = val.RuntimeFunction;
        break;
    case ValueType::kNULL:
        Integer = 0;
        break;
    default:
        throw RuntimeException("unknown value type!!!");
    }
    Type = val.Type;
    return *this;
}

ArrayObject* ArrayObject::Clone() {
    ArrayObject* pNew = new ArrayObject();
    for (auto v : _array) {
        pNew->_array.push_back(v.Clone());
    }
    return pNew;
}

MapObject* MapObject::Clone() {
    MapObject* pNew = new MapObject();
    for (auto v : _map) {
        pNew->_map[v.first.Clone()] = v.second.Clone();
    }
    return pNew;
}

Value Value::Clone() const {
    switch (Type) {
    case ValueType::kArray: {
        Value ret = Value();
        ret.Type = ValueType::kArray;
        ret.object = Array()->Clone();
        return ret;
    }

    case ValueType::kMap: {
        Value ret = Value();
        ret.Type = ValueType::kMap;
        ret.object = Map()->Clone();
        return ret;
    }
    default:
        return Value(*this);
    }
}

Value::Value(Resource* res)
        : Type(ValueType::kResource), Integer(0), bytes(), resource(res), object(NULL) {}
Value::Value(const std::vector<Value>& val) : Integer(0), bytes(), resource(NULL) {
    Type = ValueType::kArray;
    scoped_refptr<ArrayObject> ptr = new ArrayObject();
    ptr->_array = val;
    object = ptr.get();
}

Value Value::make_array() {
    Value ret = Value();
    ret.Type = ValueType::kArray;
    ret.object = new ArrayObject();
    return ret;
}
Value Value::make_bytes(std::string val) {
    Value ret = Value(val);
    ret.Type = ValueType::kBytes;
    return ret;
}
Value Value::make_map() {
    Value ret = Value();
    ret.Type = ValueType::kMap;
    ret.object = new MapObject();
    return ret;
}

Value& Value::operator+=(const Value& right) {
    if (IsSameType(right)) {
        if (Type == ValueType::kString || Type == ValueType::kBytes) {
            bytes += right.bytes;
            return *this;
        }
    }
    if (IsNumber() && right.IsNumber()) {
        if (IsInteger()) {
            Integer += right.ToInteger();
        } else {
            Float += right.ToFloat();
        }
        return *this;
    }
    if (Type == ValueType::kString) {
        switch (right.Type) {
        case ValueType::kString:
            this->bytes += right.bytes;
            return *this;
        case ValueType::kFloat:
        case ValueType::kInteger:
            this->bytes += right.ToString();
            return *this;
        case ValueType::kByte:
            this->bytes += (char)right.Byte;
            return *this;
        default:
            DEBUG_CONTEXT();
            throw RuntimeException("+= operation not avaliable for right value ");
        }
    }
    if (Type == ValueType::kBytes) {
        switch (right.Type) {
        case ValueType::kBytes:
            this->bytes += right.bytes;
            return *this;
        case ValueType::kFloat:
        case ValueType::kInteger:
            this->bytes += (unsigned char)right.ToInteger();
            return *this;
        case ValueType::kByte:
            this->bytes += (char)right.Byte;
            return *this;
        default:
            DEBUG_CONTEXT();
            throw RuntimeException("+= operation not avaliable for right value ");
        }
    }
    DEBUG_CONTEXT();
    throw Interpreter::RuntimeException("+= can't apply on this value");
}

std::string Value::ToString() const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToString();
    case ValueType::kBytes:
    case ValueType::kString:
        return bytes;
    case ValueType::kNULL:
        return "nil";
    case ValueType::kInteger:
    case ValueType::kByte:
        return Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return Interpreter::ToString(Float);
    case ValueType::kResource:
        return "resource@" + Interpreter::ToString((int64_t)resource.get());

    default:
        return "unknown";
    }
}
std::string Value::ToJSONString(bool escape) const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToJSONString();
    case ValueType::kBytes:
    case ValueType::kString:
        return "\"" + EncodeJSONString(bytes, escape) + "\"";
    case ValueType::kInteger:
    case ValueType::kByte:
    case ValueType::kFloat:
        return ToString();
    default:
        return "null";
    }
}

double Value::ToFloat() const {
    if (Type == ValueType::kFloat) {
        return Float;
    }
    if (IsInteger()) {
        return (double)Integer;
    }
    return 0;
}
Value::INTVAR Value::ToInteger() const {
    if (Type == ValueType::kFloat) {
        return (INTVAR)Float;
    }
    if (IsInteger()) {
        return Integer;
    }
    return 0;
}

std::string Value::MapKey() const {
    if (IsObject()) {
        return object->MapKey();
    }
    switch (Type) {
    case ValueType::kBytes:
    case ValueType::kString:
        return bytes;
    case ValueType::kNULL:
        return "nil@nil";
    case ValueType::kInteger:
    case ValueType::kByte:
        return "integer@" + Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return "float@" + Interpreter::ToString(Float);
    case ValueType::kResource:
        return "resource@" + Interpreter::ToString((int64_t)resource.get());
    default:
        return "unknown";
    }
}
size_t Value::Length() const {
    if (IsStringOrBytes()) {
        return bytes.size();
    }
    if (Type == ValueType::kArray) {
        return Array()->_array.size();
    }
    if (Type == ValueType::kMap) {
        return Map()->_map.size();
    }
    DEBUG_CONTEXT();
    throw Interpreter::RuntimeException("this value type not have length ");
}
bool Value::ToBoolean() const {
    if (Type == ValueType::kNULL) {
        return false;
    }
    if (Type == ValueType::kFloat) {
        return ToFloat() != 0.0;
    }
    if (IsInteger()) {
        return ToInteger() != 0;
    }
    return true;
}

const Value Value::operator[](const Value& key) const {
    if (IsStringOrBytes()) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= bytes.size()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("index of string(bytes) out of range");
        }
        return Value(bytes[key.Integer]);
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("index of array out of range ( " + ToString() +
                                                "," + key.ToString() + " )");
        }
        return Array()->_array[key.Integer];
    }
    if (Type == ValueType::kMap) {
        return Map()->_map[key.MapKey()];
    }
    DEBUG_CONTEXT();
    throw Interpreter::RuntimeException("value type <" + ValueType::ToString(Type) + " :" +
                                        ToString() + "> not support index operation");
}

Value Value::Slice(const Value& f, const Value& t) const {
    size_t from = 0, to = 0;
    if (!IsStringOrBytes() && Type != ValueType::kArray) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("the value type must slice able");
    }
    if (f.Type == ValueType::kNULL) {
        from = 0;
    } else if (f.Type == ValueType::kInteger) {
        from = f.Integer;
    } else {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (t.Type == ValueType::kNULL) {
        to = Length();
    } else if (t.Type == ValueType::kInteger) {
        to = t.Integer;
    } else {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (to > Length() || from > Length()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("index out of range");
    }

    if (Type == ValueType::kString) {
        std::string sub = bytes.substr(from, to - from);
        return Value(sub);
    }
    if (Type == ValueType::kBytes) {
        std::string sub = bytes.substr(from, to - from);
        return make_bytes(sub);
    }
    Value ret = make_array();
    std::vector<Value> result;
    std::vector<Value>& array = Array()->_array;
    for (size_t i = from; i < to; i++) {
        ret.Array()->_array.push_back(array[i]);
    }
    return ret;
}
Value& Value::operator[](const Value& key) {
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("index of array out of range ( " + ToString() +
                                                "," + key.ToString() + " )");
        }
        return _array()[key.Integer];
    }
    if (Type == ValueType::kMap) {
        return _map()[key.MapKey()];
    }
    DEBUG_CONTEXT();
    throw RuntimeException("the value type <" + ValueType::ToString(Type) + " :" + ToString() +
                           "> not have value[index]= val operation");
}

void Value::SetValue(const Value& key, const Value& val) {
    if (IsStringOrBytes()) {
        if (!val.IsInteger()) {
            DEBUG_CONTEXT();
            Interpreter::RuntimeException("set the value must a integer");
        }
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= bytes.size()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set index of string(bytes) out of range");
        }
        bytes[key.Integer] = val.ToInteger() & 0xFF;
        return;
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set index of array out of range ( " + ToString() +
                                                "," + key.ToString() + " )");
        }
        Array()->_array[key.Integer] = val;
        return;
    }
    if (Type == ValueType::kMap) {
        Map()->_map[key.MapKey()] = val;
        return;
    }
    DEBUG_CONTEXT();
    throw RuntimeException("set the value type <" + ValueType::ToString(Type) + " :" + ToString() +
                           "> not have value[index]= val operation");
}

Value operator+(const Value& left, const Value& right) {
    if (left.IsStringOrBytes()) {
        switch (right.Type) {
        case ValueType::kByte: {
            Value ret(left.bytes + (char)right.Byte);
            ret.Type = left.Type;
            return ret;
        }
        case ValueType::kBytes:
        case ValueType::kInteger:
        case ValueType::kString:
        case ValueType::kFloat: {
            Value ret(left.ToString() + right.ToString());
            ret.Type = left.Type;
            return ret;
        }
        case ValueType::kNULL:
            //DEBUG_CONTEXT();
            LOG("nil warning!!!!" + left.ToString());
            return left;

        default:
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("+ operation not avaliable for this value ( " +
                                                left.ToString() + "," + right.ToString() + " )");
        }
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("+ operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() + right.ToFloat());
    }
    return Value(left.ToInteger() + right.ToInteger());
}

Value operator-(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("- operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() - right.ToFloat());
    }
    return Value(left.ToInteger() - right.ToInteger());
}
Value operator*(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("* operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() * right.ToFloat());
    }
    return Value(left.ToInteger() * right.ToInteger());
}
Value operator/(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("/ operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() / right.ToFloat());
    }
    return Value(left.ToInteger() / right.ToInteger());
}

Value operator%(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("% operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return Value(left.ToInteger() % right.ToInteger());
}

bool operator<(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes < right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("< operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() < right.ToFloat();
}
bool operator<=(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes <= right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("<= operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() < right.ToFloat();
}
bool operator>(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes > right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("> operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() > right.ToFloat();
}
bool operator>=(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes >= right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException(">= operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() >= right.ToFloat();
}

bool operator==(const Value& left, const Value& right) {
    if (left.IsNumber() && right.IsNumber()) {
        return left.ToFloat() == right.ToFloat();
    }
    if (left.IsStringOrBytes() && right.IsStringOrBytes()) {
        return left.bytes == right.bytes;
    }
    //if (left.IsStringOrBytes() || right.IsStringOrBytes()) {
    //    if (left.IsInteger() || right.IsInteger()) {
    //        return left.ToString() == right.ToString();
    // //   }
    //}
    if (!left.IsSameType(right)) {
        return false;
    }
    switch (left.Type) {
    case ValueType::kNULL:
        return true;
    case ValueType::kResource:
        return left.resource.get() == right.resource.get();
    case ValueType::kArray: {
        ArrayObject* ptr = (ArrayObject*)left.object.get();
        ArrayObject* src = (ArrayObject*)right.object.get();
        return ptr->_array == src->_array;
    }
    case ValueType::kMap: {
        MapObject* ptr = (MapObject*)left.object.get();
        MapObject* src = (MapObject*)right.object.get();
        return ptr->_map == src->_map;
    }

    default:
        return false;
    }
}

bool operator!=(const Value& left, const Value& right) {
    return !(left == right);
}

Value operator|(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("| operation only can used on Integer ");
    }
    return Value(left.Integer | right.Integer);
}

Value operator&(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("& operation only can used on Integer ");
    }
    return Value(left.Integer & right.Integer);
}
Value operator^(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("^ operation only can used on Integer ");
    }
    return Value(left.Integer ^ right.Integer);
}
Value operator<<(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("<< operation only can used on Integer ");
    }
    return Value(left.Integer << right.Integer);
}
Value operator>>(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException(">> operation only can used on Integer ");
    }
    return Value(left.Integer >> right.Integer);
}
} // namespace Interpreter