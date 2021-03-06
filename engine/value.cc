#include "value.hpp"

#include <string.h>

#include <sstream>

#include "utf8.hpp"

namespace Interpreter {
AtomInt Status::sResourceCount;
AtomInt Status::sArrayCount;
AtomInt Status::sMapCount;
AtomInt Status::sParserCount;
AtomInt Status::sScriptCount;
AtomInt Status::sVMContextCount;
AtomInt Status::sUDObjectCount;

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

std::string ToString(long long val) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%lld", val);
    return buffer;
}

std::string AddressString(const void* addr) {
    char buffer[16] = {0};
    snprintf(buffer, 16, "%llx", (long long)addr);
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
        for (size_t i = m; i < pattern.size(); i++) {
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
        o << iter->first.ToDescription();
        o << ":";
        o << iter->second.ToDescription();
        first = false;
        iter++;
    }
    o << "}";
    return o.str();
}

//Value construct
Value::Value(char val) {
    Reset();
    Type = ValueType::kByte;
    Byte = val;
}
Value::Value(unsigned char val) {
    Reset();
    Type = ValueType::kByte;
    Byte = val;
}

Value::Value() {
    Reset();
}

Value::Value(bool val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}

Value::Value(long val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}

Value::Value(int val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}

Value::Value(INTVAR val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}

Value::Value(unsigned int val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}

#ifndef WIN_386
Value::Value(size_t val) {
    Reset();
    Type = ValueType::kInteger;
    Integer = val;
}
#endif

Value::Value(double val) {
    Reset();
    Type = ValueType::kFloat;
    Float = val;
}

Value::Value(const char* str) {
    Reset();
    Type = ValueType::kString;
    text = str;
}
Value::Value(std::string str) {
    Reset();
    Type = ValueType::kString;
    text = str;
}

Value::Value(const Instruction* f) {
    Reset();
    Type = ValueType::kFunction;
    Function = f;
}

Value::Value(RUNTIME_FUNCTION f) {
    Reset();
    Type = ValueType::kRuntimeFunction;
    RuntimeFunction = f;
}

Value::Value(Resource* res) {
    Reset();
    Type = ValueType::kResource;
    resource = res;
}

Value::Value(const std::vector<Value>& val) {
    Reset();
    Type = ValueType::kArray;
    scoped_refptr<ArrayObject> ptr = new ArrayObject();
    ptr->_array = val;
    object = ptr.get();
}

Value::Value(const Value& val) {
    Reset();
    Type = val.Type;
    switch (val.Type) {
    case ValueType::kBytes:
        bytesView = val.bytesView;
        break;
    case ValueType::kString:
        text = val.text;
        break;
    case ValueType::kResource:
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
        break;
    default:
        throw RuntimeException("unknown value type!!!");
    }
}

Value& Value::operator=(const Value& val) {
    Reset();
    Type = val.Type;
    switch (val.Type) {
    case ValueType::kBytes:
        bytesView = val.bytesView;
        break;
    case ValueType::kString:
        text = val.text;
        break;
    case ValueType::kResource:
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
        break;
    default:
        throw RuntimeException("unknown value type!!!");
    }
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
        return Value::MakeBytes(bytesView.ToString());
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
    case ValueType::kObject: {
        Value ret = Value();
        ret.Type = ValueType::kObject;
        ret.object = Object()->Clone();
        return ret;
    }
    default:
        return Value(*this);
    }
}

Value Value::MakeArray() {
    Value ret = Value();
    ret.Type = ValueType::kArray;
    ret.object = new ArrayObject();
    return ret;
}
Value Value::MakeBytes(size_t Len) {
    Value ret = Value();
    ret.Type = ValueType::kBytes;
    ret.bytesView = BytesView(Len);
    return ret;
}
Value Value::MakeBytes(std::string src) {
    Value ret = Value();
    ret.Type = ValueType::kBytes;
    ret.bytesView = BytesView(src.size());
    ret.bytesView.CopyFrom(src);
    return ret;
}
Value Value::MakeMap() {
    Value ret = Value();
    ret.Type = ValueType::kMap;
    ret.object = new MapObject();
    return ret;
}

Value Value::MakeObject(UDObject* obj) {
    Value ret = Value();
    ret.Type = ValueType::kObject;
    ret.object = obj;
    return ret;
}

Value& Value::operator+=(const Value& right) {
    if (right.IsNULL()) {
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
    if (IsSameType(right) && IsString()) {
        text += right.text; //string + string
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
    DUMP_CONTEXT();
    NVT_LOG_ERROR(ToDescription(), right.ToDescription());
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
        std::string ret = "";
        ret.append(1, (char)Byte);
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

bool IsDigestString(const std::string& src) {
    if (src.size() == 0) {
        return false;
    }
    for (size_t i = 0; i < src.size(); i++) {
        int c = ((int)src[i]) & 0xFF;
        if (c < '0' || c > '9') {
            if (i == 0 && c == '-') {
                continue;
            }
            return false;
        }
    }
    return true;
}

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
            return ValueType::ToString(Type) + "(" + text + ")";
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
    if (IsArray() || IsMap() || IsObject()) {
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

std::string Value::TypeName() const {
    if (IsObject()) {
        return object->TypeName();
    }
    std::string name;
    if (IsResource(name)) {
        return name;
    }
    return ValueType::ToString(Type);
};

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
    if (IsObject()) {
        return Object()->__length();
    }
    DUMP_CONTEXT();
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
            if (text == "0") {
                return false;
            }
        }
        return true;
    }
    if (IsBytes()) {
        return bytesView.Length() > 0;
    }
    return true;
}

const Value Value::operator[](const Value& key) const {
    if (IsBytes()) {
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("index of bytes out of range");
        }
        return Value((char)bytesView.GetAt((size_t)key.Integer));
    }
    if (IsString()) {
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= text.size()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("index of string out of range");
        }
        return Value((BYTE)text[(size_t)key.Integer]);
    }
    if (IsArray()) {
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            if (key.Integer > 4096) {
                DUMP_SHORT_STACK();
                NVT_LOG_WARNING("use big array large than 4096");
            }
            Array()->_array.resize((size_t)key.Integer + 1);
        }
        return Array()->_array[(size_t)key.Integer];
    }
    if (IsMap()) {
        auto iter = Map()->_map.find(key);
        if (iter == Map()->_map.end()) {
            return Value();
        }
        return iter->second;
    }
    if (IsObject()) {
        return Object()->__get_attr(key);
    }
    DUMP_CONTEXT();
    throw Interpreter::RuntimeException("value type <" + ToDescription() +
                                        "> not support index operation");
}

Value& Value::operator[](const Value& key) {
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            NVT_LOG_DEBUG("index of array out of range ( " + ToString() + "," + key.ToString() +
                          " ) auto extend");
            if (key.Integer > 4096) {
                DUMP_SHORT_STACK();
                NVT_LOG_WARNING("use big array large than 4096");
            }
            Array()->_array.resize((size_t)key.Integer + 1);
        }
        return _array()[(size_t)key.Integer];
    }
    if (Type == ValueType::kMap) {
        return _map()[key];
    }
    DUMP_CONTEXT();
    throw RuntimeException("the value type <" + ToDescription() +
                           "> not have value[index]= val operation");
}

Value Value::GetValue(const Value& key) const {
    return (*this)[key];
}

void Value::SetValue(const Value& key, const Value& val) {
    if (IsBytes()) {
        if (!val.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set the value must a integer");
        }
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= bytesView.Length()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set index of bytes out of range");
        }
        if (val.IsInteger()) {
            bytesView.SetAt((size_t)key.Integer, (int)val.ToInteger());
            return;
        }
    }
    if (IsString()) {
        if (!val.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set the value must a integer");
        }
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set index of string(bytes) out of range");
        }
        text[(size_t)key.Integer] = val.ToInteger() & 0xFF;
        return;
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            DUMP_CONTEXT();
            throw Interpreter::RuntimeException("set the index key type must a Integer");
        }
        if (key.Integer < 0 || (size_t)key.Integer >= Length()) {
            if (key.Integer > 4096) {
                DUMP_SHORT_STACK();
                NVT_LOG_WARNING("use big array large than 4096");
            }
            Array()->_array.resize((size_t)key.Integer + 1);
        }
        Array()->_array[(size_t)key.Integer] = val;
        return;
    }
    if (Type == ValueType::kMap) {
        Map()->_map[key] = val;
        return;
    }
    if (IsObject()) {
        Object()->__set_attr(key, val);
        return;
    }
    DUMP_CONTEXT();
    throw RuntimeException("set the value type <" + ToDescription() +
                           "> not have value[index]= val operation");
}

Value Value::Slice(const Value& f, const Value& t) const {
    size_t from = 0, to = 0;
    if (!(IsBytes() || IsString()) && Type != ValueType::kArray) {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("the value type must slice able");
    }
    if (f.Type == ValueType::kNULL) {
        from = 0;
    } else if (f.Type == ValueType::kInteger) {
        from = (size_t)f.Integer;
    } else {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (t.Type == ValueType::kNULL) {
        to = Length();
    } else if (t.Type == ValueType::kInteger) {
        to = (size_t)t.Integer;
    } else {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (to > Length() || from > Length()) {
        DUMP_CONTEXT();
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
    Value ret = MakeArray();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        DUMP_CONTEXT();
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
        if (right.Integer == 0) {
            return left.ToBoolean() == false;
        }
        if (right.IsByte()) {
            return left.text == right.ToString();
        }
        if (!IsDigestString(left.text)) {
            DUMP_CONTEXT();
            NVT_LOG_WARNING("compare string with integer,may be have bug <always false>",
                            left.ToDescription(), "<-->", right.ToDescription());
        }
        return left.text == right.ToString();
    }
    if (right.IsString() && left.IsInteger()) {
        if (left.Integer == 0) {
            return right.ToBoolean() == false;
        }
        if (left.IsByte()) {
            return right.text == left.ToString();
        }
        if (!IsDigestString(right.text)) {
            DUMP_CONTEXT();
            NVT_LOG_WARNING("compare string with integer,may be have bug <always false>",
                            left.ToDescription(), "<-->", right.ToDescription());
        }
        return right.text == left.ToString();
    }
    if (!left.IsSameType(right)) {
        NVT_LOG_WARNING("compare value not same type may be have bug <always false> ",
                        left.ToDescription(), " ", right.ToDescription());
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

    case ValueType::kObject: {
        return left.Object()->__equal(right.Object());
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
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("| operation only can used on Integer ");
    }
    return Value(left.Integer | right.Integer);
}

Value operator&(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("& operation only can used on Integer ");
    }
    return Value(left.Integer & right.Integer);
}
Value operator^(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("^ operation only can used on Integer ");
    }
    return Value(left.Integer ^ right.Integer);
}
Value operator<<(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException("<< operation only can used on Integer ");
    }
    return Value(left.Integer << right.Integer);
}
Value operator>>(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        DUMP_CONTEXT();
        throw Interpreter::RuntimeException(">> operation only can used on Integer ");
    }
    return Value(left.Integer >> right.Integer);
}
} // namespace Interpreter