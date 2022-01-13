#include <sstream>

#include "value.hpp"
#include "vm.hpp"
namespace Interpreter {
UDObject::UDObject(Executor* Engine, const std::string& Type,
                   std::map<std::string, Value>& attributes)
        : mEngine(Engine), mTypeName(Type), mAttributes(attributes) {
            Status::sUDObjectCount++;
        }

UDObject::~UDObject(){
    Status::sUDObjectCount--;
}

std::string UDObject::ToString() const {
    std::stringstream o;
    o << "{";
    for (auto iter : mAttributes) {
        o << iter.first;
        o << ":";
        o << iter.second.ToString();
        o << ",";
    }
    std::string res = o.str();
    if (res.size() > 1) {
        res[res.size() - 1] = '}';
    } else {
        res += "}";
    }
    return res;
}

std::string UDObject::ToDescription() const {
    std::stringstream o;
    o << "{";
    for (auto iter : mAttributes) {
        o << iter.first;
        o << ":";
        o << iter.second.ToDescription();
        o << ",";
    }
    std::string res = o.str();
    if (res.size() > 1) {
        res[res.size() - 1] = '}';
    } else {
        res += "}";
    }
    return res;
}

std::string UDObject::ToJSONString() const {
    std::stringstream o;
    o << "{";
    for (auto iter : mAttributes) {
        o << "\"";
        o << iter.first;
        o << "\"";
        o << ":";
        o << iter.second.ToJSONString(true);
        o << ",";
    }
    std::string res = o.str();
    if (res.size() > 1) {
        res[res.size() - 1] = '}';
    } else {
        res += "}";
    }
    return res;
}

Value UDObject::__get_attr(Value key) const {
    if (key.IsString()) {
        auto iter = mAttributes.find(key.text);
        if (iter != mAttributes.end()) {
            return iter->second;
        }
    }
    Value func = mEngine->GetFunction(mTypeName + "#__get_index__", NULL);
    if (func.IsNULL()) {
        return Value();
    }
    std::vector<Value> values;
    values.push_back(key);
    return mEngine->CallObjectMethod(Value::MakeObject((UDObject*)this), func, values, NULL);
}

Value UDObject::__set_attr(Value key, Value val) {
    if (key.IsString()) {
        auto iter = mAttributes.find(key.text);
        if (iter != mAttributes.end()) {
            mAttributes[key.text] = val;
            return val;
        }
    }

    Value func = mEngine->GetFunction(mTypeName + "#__set_index__", NULL);
    if (func.IsNULL()) {
        return Value();
    }
    std::vector<Value> values;
    values.push_back(key);
    values.push_back(val);
    return mEngine->CallObjectMethod(Value::MakeObject((UDObject*)this), func, values, NULL);
}

std::list<std::pair<Value, Value>> UDObject::__enum_all() const {
    std::list<std::pair<Value, Value>> res;
    Value func = mEngine->GetFunction(mTypeName + "#__enum_all__", NULL);
    if (func.IsNULL()) {
        for (auto iter : mAttributes) {
            res.push_back(iter);
        }
        return res;
    }
    std::vector<Value> values;
    Value result =
            mEngine->CallObjectMethod(Value::MakeObject((UDObject*)this), func, values, NULL);
    if (result.IsArray()) {
        for (auto v : result._array()) {
            if (v.IsMap()) {
                res.push_back(std::make_pair(v["__key__"], v["__value__"]));
            }
        }
    }
    return res;
}

UDObject* UDObject::Clone() const {
    std::map<std::string, Value> attrs;
    for (auto iter : mAttributes) {
        attrs[iter.first] = iter.second.Clone();
    }
    return new UDObject(mEngine, mTypeName, attrs);
}

bool UDObject::__equal(const UDObject* other) const {
    for (auto iter : mAttributes) {
        if (iter.second != other->__get_attr(iter.first)) {
            return false;
        }
    }
    return true;
}

size_t UDObject::__length() const {
    Value func = mEngine->GetFunction(mTypeName + "#__len__", NULL);
    if (func.IsNULL()) {
        return mAttributes.size();
    }
    std::vector<Value> values;
    return (size_t)mEngine->CallObjectMethod(Value::MakeObject((UDObject*)this), func, values, NULL)
            .ToInteger();
}
} // namespace Interpreter