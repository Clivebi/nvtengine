#include "value.hpp"

#include <sstream>
namespace Interpreter {

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
    result.push_back(part);
    return result;
}

std::string HTMLEscape(const std::string& src) {
    const char* hex = "0123456789abcdef";
    std::stringstream o;
    int start = 0;
    for (size_t i = 0; i < src.size(); i++) {
        int c = src[i];
        if (c == '<' || c == '>' || c == '&') {
            o << "\\u00";
            o << hex[c >> 4];
            o << hex[c & 0xF];
            start = i + 1;
        }
        if (c == 0xE2 && i + 2 < src.size() && (int)src[i + 1] == 0x80 &&
            (src[i + 2] & (src[i + 2] ^ 1)) == 0xA8) {
            if (start < i) {
                o << src.substr(start, i - start);
            }
            o << "\\u202";
            o << hex[src[i + 2] & 0xF];
            start = i + 3;
        }
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
        default:
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
        default:
            throw RuntimeException("+= operation not avaliable for right value ");
        }
    }
    throw Interpreter::RuntimeException("+= cant applay on this value");
}

std::string Value::ToString() const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToString();
    case ValueType::kBytes:
        return HexEncode(bytes.c_str(), bytes.size());
    case ValueType::kString:
        return bytes;
    case ValueType::kNULL:
        return "nil";
    case ValueType::kInteger:
        return Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return Interpreter::ToString(Float);
    case ValueType::kResource:
        return "resource@" + Interpreter::ToString((int64_t)resource.get());

    default:
        return "unknown";
    }
}
std::string Value::ToJSONString() const {
    switch (Type) {
    case ValueType::kArray:
    case ValueType::kMap:
    case ValueType::kObject:
        return object->ToJSONString();
    case ValueType::kBytes:
    case ValueType::kString:
        return "\"" + HTMLEscape(bytes) + "\"";
    default:
        return "null";
    }
}

double Value::ToFloat() const {
    if (Type == ValueType::kFloat) {
        return Float;
    }
    if (Type == ValueType::kInteger) {
        return (double)Integer;
    }
    return 0;
}
Value::INTVAR Value::ToInteger() const {
    if (Type == ValueType::kFloat) {
        return (INTVAR)Float;
    }
    if (Type == ValueType::kInteger) {
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
        return "integer@" + Interpreter::ToString(Integer);
    case ValueType::kFloat:
        return "float@" + Interpreter::ToString(Float);
    case ValueType::kResource:
        return "resource@" + Interpreter::ToString((int64_t)resource.get());
    default:
        return "unknown";
    }
}
size_t Value::Length() {
    if (IsStringOrBytes()) {
        return bytes.size();
    }
    if (Type == ValueType::kArray) {
        return _array().size();
    }
    if (Type == ValueType::kMap) {
        return _map().size();
    }
    throw Interpreter::RuntimeException("this value type not have length ");
}
bool Value::ToBoolean() {
    if (Type == ValueType::kNULL) {
        return false;
    }
    if (Type == ValueType::kFloat) {
        return ToFloat() != 0.0;
    }
    if (Type == ValueType::kInteger) {
        return ToInteger() != 0;
    }
    return true;
}

Value Value::operator[](const Value& key) {
    if (IsStringOrBytes()) {
        if (!key.IsInteger()) {
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= bytes.size()) {
            throw Interpreter::RuntimeException("index of string(bytes) out of range");
        }
        return Value((long)(bytes[key.Integer]) & 0xFF);
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= Length()) {
            throw Interpreter::RuntimeException("index of array out of range");
        }
        return Array()->_array[key.Integer];
    }
    if (Type == ValueType::kMap) {
        return Map()->_map[key.MapKey()];
    }
    throw Interpreter::RuntimeException("value not support index operation");
}

Value Value::Slice(const Value& f, const Value& t) {
    size_t from = 0, to = 0;
    if (!IsStringOrBytes() && Type != ValueType::kArray) {
        throw Interpreter::RuntimeException("the value type must slice able");
    }
    if (f.Type == ValueType::kNULL) {
        from = 0;
    } else if (f.Type == ValueType::kInteger) {
        from = f.Integer;
    } else {
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (t.Type == ValueType::kNULL) {
        to = Length();
    } else if (t.Type == ValueType::kInteger) {
        to = t.Integer;
    } else {
        throw Interpreter::RuntimeException("the index key type must a Integer");
    }
    if (to > Length() || from > Length()) {
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
    std::vector<Value>& array = _array();
    for (size_t i = from; i < to; i++) {
        ret.Array()->_array.push_back(array[i]);
    }
    return ret;
}

void Value::SetValue(const Value& key, const Value& val) {
    if (IsStringOrBytes()) {
        if (!val.IsInteger()) {
            Interpreter::RuntimeException("the value must a integer");
        }
        if (!key.IsInteger()) {
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= bytes.size()) {
            throw Interpreter::RuntimeException("index of string(bytes) out of range");
        }
        bytes[key.Integer] = val.ToInteger() & 0xFF;
        return;
    }
    if (Type == ValueType::kArray) {
        if (!key.IsInteger()) {
            throw Interpreter::RuntimeException("the index key type must a Integer");
        }
        if (key.Integer < 0 || key.Integer >= Length()) {
            throw Interpreter::RuntimeException("index of array out of range");
        }
        Array()->_array[key.Integer] = val;
        return;
    }
    if (Type == ValueType::kMap) {
        Map()->_map[key.MapKey()] = val;
        return;
    }
    throw RuntimeException("the value type not have value[index]= val operation");
}

Value operator+(const Value& left, const Value& right) {
    if (left.IsSameType(right) &&
        (ValueType::kString == left.Type || ValueType::kBytes == left.Type)) {
        Value ret = Value::make_bytes(left.bytes + right.bytes);
        ret.Type = left.Type;
        return ret;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("arithmetic operation not avaliable for this value ");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() + right.ToFloat());
    }
    return Value(left.ToInteger() + right.ToInteger());
}

Value operator-(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("arithmetic operation not avaliable for this value ");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() - right.ToFloat());
    }
    return Value(left.ToInteger() - right.ToInteger());
}
Value operator*(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("arithmetic operation not avaliable for this value ");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() * right.ToFloat());
    }
    return Value(left.ToInteger() * right.ToInteger());
}
Value operator/(const Value& left, const Value& right) {
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("arithmetic operation not avaliable for this value ");
    }
    if (left.Type == ValueType::kFloat || right.Type == ValueType::kFloat) {
        return Value(left.ToFloat() / right.ToFloat());
    }
    return Value(left.ToInteger() / right.ToInteger());
}

Value operator%(const Value& left, const Value& right) {
    if (!left.IsInteger() || !right.IsInteger()) {
        throw Interpreter::RuntimeException("% operation not avaliable for this value ");
    }
    return Value(left.ToInteger() & right.ToInteger());
}

bool operator<(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes < right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("< operation not avaliable for this value ");
    }
    return left.ToFloat() < right.ToFloat();
}
bool operator<=(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes <= right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("<= operation not avaliable for this value ");
    }
    return left.ToFloat() < right.ToFloat();
}
bool operator>(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes > right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException("> operation not avaliable for this value ");
    }
    return left.ToFloat() > right.ToFloat();
}
bool operator>=(const Value& left, const Value& right) {
    if (left.Type == ValueType::kString && right.Type == ValueType::kString) {
        return left.bytes >= right.bytes;
    }
    if (!left.IsNumber() || !right.IsNumber()) {
        throw Interpreter::RuntimeException(">= operation not avaliable for this value ");
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
    if (left.Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
        throw Interpreter::RuntimeException("| operation only can used on Integer ");
    }
    return Value(left.Integer | right.Integer);
}

Value operator&(const Value& left, const Value& right) {
    if (left.Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
        throw Interpreter::RuntimeException("& operation only can used on Integer ");
    }
    return Value(left.Integer & right.Integer);
}
Value operator^(const Value& left, const Value& right) {
    if (left.Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
        throw Interpreter::RuntimeException("^ operation only can used on Integer ");
    }
    return Value(left.Integer ^ right.Integer);
}
Value operator<<(const Value& left, const Value& right) {
    if (left.Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
        throw Interpreter::RuntimeException("<< operation only can used on Integer ");
    }
    return Value(left.Integer << right.Integer);
}
Value operator>>(const Value& left, const Value& right) {
    if (left.Type != ValueType::kInteger || right.Type != ValueType::kInteger) {
        throw Interpreter::RuntimeException(">> operation only can used on Integer ");
    }
    return Value(left.Integer >> right.Integer);
}
} // namespace Interpreter