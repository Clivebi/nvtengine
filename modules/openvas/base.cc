#include "api.hpp"
void OVAContext::AddPreference(const Value& id, const Value& name, const Value& type,
                               const Value& value) {
    Value x = Value::make_map();
    x["name"] = name;
    x["type"] = type;
    x["value"] = value;
    x["id"] = id;
    auto iter = Nvti._map().find("preference");
    Value newVal = Value::make_array();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()["preference"] = newVal;
    }
    if (id.Integer < 0) {
        x["id"] = pref._array().size();
    }
    //TODO: check exist preference
    pref._array().push_back(x);
}

Value OVAContext::GetPreference(const Value& id, const Value& name) {
    Value& ref = Nvti._map()["preference"];
    if (ref.Type != ValueType::kArray) {
        throw RuntimeException("preference not exist :" + name.ToString());
    }
    auto iter = ref._array().begin();
    while (iter != ref._array().end()) {
        if ((*iter)["name"] == name) {
            if (id != -1) {
                if ((*iter)["id"] == id) {
                    return (*iter)["value"];
                }
                return Value();
            }
            return (*iter)["value"];
        }
        iter++;
    }
    return Value();
}
void OVAContext::GetPreferenceFile(const Value& id, const Value& name, std::string& content) {
    content = "";
}

void OVAContext::AddXref(const Value& name, const Value& value) {
    auto iter = Nvti._map().find("xref");
    Value newVal = Value::make_map();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()["xref"] = newVal;
    }
    pref[name] = value;
}
void OVAContext::AddTag(const Value& name, const Value& value) {
    auto iter = Nvti._map().find("tag");
    Value newVal = Value::make_map();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()["tag"] = newVal;
    }
    pref[name] = value;
}