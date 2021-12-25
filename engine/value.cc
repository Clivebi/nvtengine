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

std::string HexEncode(const char* buf, size_t count, std::string prefix) {
    char buffer[6] = {0};
    std::string result = "";
    for (size_t i = 0; i < count; i++) {
        snprintf(buffer, 6, "%02x", (unsigned char)buf[i]);
        result += prefix;
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

std::string& replace_str(std::string& str, const std::string& to_replaced,
                         const std::string& newchars, int maxcount) {
    int count = 0;
    if (to_replaced.size() == 0 || str.size() == 0) {
        return str;
    }
    for (std::string::size_type pos(0); pos != std::string::npos; pos += newchars.length()) {
        pos = str.find(to_replaced, pos);
        if (pos != std::string::npos) {
            str.replace(pos, to_replaced.length(), newchars);
            count++;
            if (maxcount != -1 && maxcount == count) {
                break;
            }
        } else {
            break;
        }
    }
    return str;
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

std::string ArrayObject::ToDescription() const {
    std::stringstream o;
    bool first = true;
    o << "[";
    auto iter = _array.begin();
    while (iter != _array.end()) {
        if (!first) {
            o << ",";
        }
        o << (*iter).ToDescription();
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

std::string MapObject::ToDescription() const {
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
        o << iter->second.ToDescription();
        first = false;
        iter++;
    }
    o << "}";
    return o.str();
}

//Value 众多的构造函数
Value::Value(char val)
        : Type(ValueType::kByte), Byte(val), bytesView(), text(), resource(NULL), object(NULL) {
    Integer = 0;
    Byte = val;
}
Value::Value(unsigned char val)
        : Type(ValueType::kByte), Byte(val), bytesView(), text(), resource(NULL), object(NULL) {
    Integer = 0;
    Byte = val;
}

Value::Value()
        : Type(ValueType::kNULL), Integer(0), bytesView(), text(), resource(NULL), object(NULL) {}

Value::Value(bool val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(long val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(int val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(INTVAR val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(unsigned int val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(size_t val)
        : Type(ValueType::kInteger),
          Integer(val),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(double val)
        : Type(ValueType::kFloat), Float(val), bytesView(), text(), resource(NULL), object(NULL) {}
Value::Value(const char* str)
        : Type(ValueType::kString),
          Integer(0),
          bytesView(),
          text(str),
          resource(NULL),
          object(NULL) {}
Value::Value(std::string str)
        : Type(ValueType::kString),
          Integer(0),
          bytesView(),
          text(str),
          resource(NULL),
          object(NULL) {}

Value::Value(const Instruction* f)
        : Type(ValueType::kFunction),
          Function(f),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}
Value::Value(Resource* res)
        : Type(ValueType::kResource),
          Integer(0),
          bytesView(),
          text(),
          resource(res),
          object(NULL) {}
Value::Value(const std::vector<Value>& val) : Integer(0), bytesView(), text(), resource(NULL) {
    Type = ValueType::kArray;
    scoped_refptr<ArrayObject> ptr = new ArrayObject();
    ptr->_array = val;
    object = ptr.get();
}
Value::Value(RUNTIME_FUNCTION f)
        : Type(ValueType::kRuntimeFunction),
          RuntimeFunction(f),
          bytesView(),
          text(),
          resource(NULL),
          object(NULL) {}

Value::Value(const Value& val) {
    switch (val.Type) {
    case ValueType::kBytes:
        Integer = 0;
        bytesView = val.bytesView;
        break;
    case ValueType::kString:
        Integer = 0;
        text = val.text;
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
        Integer = 0;
        bytesView = val.bytesView;
        break;
    case ValueType::kString:
        Integer = 0;
        text = val.text;
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
    case ValueType::kBytes:
        return Value::make_bytes(bytesView.ToString());
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

Value Value::make_array() {
    Value ret = Value();
    ret.Type = ValueType::kArray;
    ret.object = new ArrayObject();
    return ret;
}
Value Value::make_bytes(size_t Len) {
    Value ret = Value();
    ret.Type = ValueType::kBytes;
    ret.bytesView = BytesView(Len);
    return ret;
}
Value Value::make_bytes(std::string src) {
    Value ret = Value();
    ret.Type = ValueType::kBytes;
    ret.bytesView = BytesView(src.size());
    ret.bytesView.CopyFrom(src);
    return ret;
}
Value Value::make_map() {
    Value ret = Value();
    ret.Type = ValueType::kMap;
    ret.object = new MapObject();
    return ret;
}

Value& Value::operator+=(const Value& right) {
    if (right.IsNULL()) {
        return *this;
    }
    if (IsSameType(right) && IsString()) {
        text += right.text; //string + string
        return *this;
    }
    if (IsNumber() && right.IsNumber()) { //number + number
        if (Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
            Float = ToFloat() + right.ToFloat();
            Type = ValueType::kFloat;
        } else {
            Integer += right.ToInteger();
        }
        return *this;
    }
    if (right.IsString()) { // other + string
        std::string result = "";
        switch (Type) {
        case ValueType::kByte:
            result += (char)Byte;
            break;
        default:
            result = ToString();
        }
        result += right.text;
        Type = right.Type;
        Integer = 0;
        text = result;
        return *this;
    }
    if (IsString()) { //string + other
        switch (right.Type) {
        case ValueType::kByte:
            text += (char)right.Byte;
            break;
        default:
            text += right.ToString();
        }
        return *this;
    }
    if (IsArray()) { //array + element
        _array().push_back(right);
        return *this;
    }

    DEBUG_CONTEXT();
    LOG(ToString(), right.ToString());
    throw Interpreter::RuntimeException("+= can't apply on this value");
}

std::string Value::ToString() const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToString();
    case ValueType::kBytes:
        return bytesView.ToString();
    case ValueType::kString:
        return text;
    case ValueType::kNULL:
        return "";
    case ValueType::kInteger:
        return Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return Interpreter::ToString(Float);
    case ValueType::kByte: {
        std::string ret="";
        ret.append(1,(char)Byte);
        return ret;
    }
    default:
        return "";
    }
}

bool IsVisableString(const std::string& src) {
    for (size_t i = 0; i < src.size(); i++) {
        int c = ((int)src[i]) & 0xFF;
        if (isprint(c) || c == '\r' || c == '\n' || c == '\t' || c == ' ') {
            continue;
        }
        int d_size = 0;
        int rune = utf8::DecodeRune(src.substr(i), d_size);
        if (rune == utf8::RuneError || d_size == 1) {
            return false;
        }
        i += d_size;
        i--;
    }
    return true;
}

//描述信息
std::string Value::ToDescription() const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToDescription();
    case ValueType::kBytes:
        return bytesView.ToDescription();
    case ValueType::kString: {
        if (IsVisableString(text)) {
            return text;
        }
        return ValueType::ToString(Type) + "(" + HexEncode(text.c_str(), text.size()) + ")";
    }
    case ValueType::kNULL:
        return "nil";
    case ValueType::kByte:
    case ValueType::kInteger:
        return ValueType::ToString(Type) + "@" + Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return ValueType::ToString(Type) + "@" + Interpreter::ToString(Float);
    case ValueType::kResource:
        return "resource@" + resource->TypeName() + "@" +
               Interpreter::ToString((int64_t)resource.get());

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
        return "\"" + bytesView.ToJSONString() + "\"";
    case ValueType::kString:
        return "\"" + EncodeJSONString(text, escape) + "\"";
    case ValueType::kInteger:
    case ValueType::kByte:
    case ValueType::kFloat:
        return ToString();
    default:
        return "null";
    }
}

double Value::ToFloat() const {
    switch (Type) {
    case ValueType::kInteger:
    case ValueType::kByte:
        return (double)Integer;
    case ValueType::kFloat:
        return Float;
    case ValueType::kString:
        return strtod(text.c_str(), NULL);
    case ValueType::kBytes:
        return strtod(bytesView.ToString().c_str(), NULL);
    default:
        return 0.0;
    }
}
Value::INTVAR Value::ToInteger() const {
    switch (Type) {
    case ValueType::kInteger:
    case ValueType::kByte:
        return Integer;
    case ValueType::kFloat:
        return (Value::INTVAR)Float;
    case ValueType::kString:
        return strtoll(text.c_str(), NULL, 0);
    case ValueType::kBytes: {
        return strtoll(bytesView.ToString().c_str(), NULL, 0);
    }
    default:
        return (-1ll);
    }
}

std::string Value::MapKey() const {
    if (IsObject()) {
        return object->MapKey();
    }
    switch (Type) {
    case ValueType::kBytes:
        return bytesView.ToString();
    case ValueType::kString:
        return text;
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
    if (IsBytes()) {
        return bytesView.Length();
    }
    if (IsString()) {
        return text.size();
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
    if (IsString()) {
        if (text.size() == 0) {
            return false;
        }
        if (text.size() == 1) {
            return text == "0";
        }
    }
    if (IsBytes()) {
        return bytesView.Length() > 0;
    }
    return true;
}

const Value Value::operator[](const Value& key) const {
    if (IsBytes()) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("index of bytes out of range");
        }
        return Value((char)bytesView.GetAt(key.Integer));
    }
    if (IsString()) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= text.size()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("index of string out of range");
        }
        return Value(text[key.Integer]);
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            LOG("index of array out of range ( " + ToString() + "," + key.ToString() +
                " ) auto extend");
            if (key.Integer > 4096) {
                throw RuntimeException("index of array out of range ( " + ToString() + "," +
                                       key.ToString() + " )");
            }
            Array()->_array.resize(key.Integer + 1);
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

Value& Value::operator[](const Value& key) {
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            LOG("index of array out of range ( " + ToString() + "," + key.ToString() +
                " ) auto extend");
            if (key.Integer > 4096) {
                throw RuntimeException("index of array out of range ( " + ToString() + "," +
                                       key.ToString() + " )");
            }
            Array()->_array.resize(key.Integer + 1);
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

Value Value::GetValue(const Value& key) const {
    return (*this)[key];
}

void Value::SetValue(const Value& key, const Value& val) {
    if (IsBytes()) {
        if (!val.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the value must a integer");
        }
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= bytesView.Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set index of bytes out of range");
        }
        if (val.IsInteger()) {
            bytesView.SetAt(key.Integer, (int)val.ToInteger());
            return;
        }
    }
    if (IsString()) {
        if (!val.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the value must a integer");
        }
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set index of string(bytes) out of range");
        }
        text[key.Integer] = val.ToInteger() & 0xFF;
        return;
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DEBUG_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            LOG("set index of array out of range ( " + ToString() + "," + key.ToString() +
                " ) auto extend");
            if (key.Integer > 4096) {
                throw RuntimeException("set index of array out of range ( " + ToString() + "," +
                                       key.ToString() + " )");
            }
            Array()->_array.resize(key.Integer + 1);
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

Value Value::Slice(const Value& f, const Value& t) const {
    size_t from = 0, to = 0;
    if (!(IsBytes() || IsString()) && Type != ValueType::kArray) {
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
        std::string sub = text.substr(from, to - from);
        return Value(sub);
    }
    if (Type == ValueType::kBytes) {
        BytesView view = bytesView.Slice(from, to);
        Value ret;
        ret.bytesView = view;
        ret.Type = ValueType::kBytes;
        return ret;
    }
    Value ret = make_array();
    std::vector<Value> result;
    std::vector<Value>& array = Array()->_array;
    for (size_t i = from; i < to; i++) {
        ret.Array()->_array.push_back(array[i]);
    }
    return ret;
}

Value operator+(const Value& left, const Value& right) {
    Value result = left;
    result += right;
    return result;
}

Value operator-(const Value& left, const Value& right) {
    if (left.IsString() && right.IsString()) {
        std::string result = left.text;
        return replace_str(result, right.text, "", 1);
    }
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
    if ((left.IsString() || left.IsBytes()) && (right.IsString() || right.IsBytes())) {
        return left.ToString() < right.ToString();
    }
    if (left.IsNULL() && !right.IsNULL()) {
        return true;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("< operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() < right.ToFloat();
}
bool operator<=(const Value& left, const Value& right) {
    if ((left.IsString() || left.IsBytes()) && (right.IsString() || right.IsBytes())) {
        return left.ToString() <= right.ToString();
    }
    if (left.IsNULL() && right.IsNULL()) {
        return true;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("<= operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() <= right.ToFloat();
}
bool operator>(const Value& left, const Value& right) {
    if ((left.IsString() || left.IsBytes()) && (right.IsString() || right.IsBytes())) {
        return left.ToString() > right.ToString();
    }
    if (left.IsNULL() && !right.IsNULL()) {
        return false;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException("> operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() > right.ToFloat();
}
bool operator>=(const Value& left, const Value& right) {
    if ((left.IsString() || left.IsBytes()) && (right.IsString() || right.IsBytes())) {
        return left.ToString() >= right.ToString();
    }
    if (left.IsNULL() && right.IsNULL()) {
        return true;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        DEBUG_CONTEXT();
        throw Interpreter::RuntimeException(">= operation not avaliable for this value ( " +
                                            left.ToString() + "," + right.ToString() + " )");
    }
    return left.ToFloat() >= right.ToFloat();
}

bool operator==(const Value& left, const Value& right) {
    //number == number
    if (left.IsNumber() && right.IsNumber()) {
        return left.ToFloat() == right.ToFloat();
    }
    //string == string
    if ((left.IsString() || left.IsBytes()) && (right.IsString() || right.IsBytes())) {
        return left.ToString() == right.ToString();
    }
    if (left.IsNULL()) {
        return false == right.ToBoolean();
    }
    if (right.IsNULL()) {
        return false == left.ToBoolean();
    }
    if (left.IsString() && right.IsInteger()) {
        if (right.Type != ValueType::kByte) {
            DEBUG_CONTEXT();
            LOG("compare string with integer,may be have bug ", left.ToDescription(), " ",
                right.ToDescription());
        }

        return left.text == right.ToString();
    }
    if (right.IsString() && left.IsInteger()) {
        if (left.Type != ValueType::kByte) {
            DEBUG_CONTEXT();
            LOG("compare string with integer,may be have bug ", left.ToDescription(), " ",
                right.ToDescription());
        }
        return right.text == left.ToString();
    }
    if (!left.IsSameType(right)) {
        LOG("compare value not same type may be have bug <always false> ", left.ToDescription(),
            " ", right.ToDescription());
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