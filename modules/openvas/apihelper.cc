#include "api.hpp"

void OVAContext::AddPreference(const Value& id, const Value& name, const Value& type,
                               const Value& value) {
    Value x = Value::make_map();
    x["name"] = name;
    x["type"] = type;
    x["value"] = value;
    x["id"] = id;
    auto iter = Nvti._map().find(knowntext::kNVTI_preference);
    Value newVal = Value::make_array();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()[knowntext::kNVTI_preference] = newVal;
    }
    if (id.Integer < 0) {
        x["id"] = pref._array().size();
    }
    //TODO: check exist preference
    pref._array().push_back(x);
}

Value OVAContext::GetPreference(const Value& id, const Value& name) {
    Value& ref = Nvti._map()[knowntext::kNVTI_preference];
    if (ref.Type != ValueType::kArray) {
        LOG("preference array not exist " + Nvti[knowntext::kNVTI_filename].ToString());
        return Value();
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
    auto iter = Nvti._map().find(knowntext::kNVTI_xref);
    Value newVal = Value::make_map();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()[knowntext::kNVTI_xref] = newVal;
    }
    pref[name] = value;
}
void OVAContext::AddTag(const Value& name, const Value& value) {
    auto iter = Nvti._map().find(knowntext::kNVTI_tag);
    Value newVal = Value::make_map();
    Value& pref = newVal;
    if (iter != Nvti._map().end()) {
        pref = iter->second;
    } else {
        Nvti._map()[knowntext::kNVTI_tag] = newVal;
    }
    pref[name] = value;
}

bool OVAContext::IsPortInOpenedRange(Value& port, bool tcp) {
    std::string portList;
    if (tcp) {
        portList = Env[knowntext::kENV_opened_tcp].bytes;
    } else {
        portList = Env[knowntext::kENV_opened_udp].bytes;
    }
    std::string key = "," + port.ToString();
    if (portList.find(key) != std::string::npos) {
        return true;
    }
    //Services/wwww
    Value ret = Storage->GetItem(port.ToString(), false);
    return !ret.IsNULL();
}