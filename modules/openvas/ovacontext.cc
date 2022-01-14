#include "ovacontext.hpp"
void OVAContext::AddPreference(const Value& id, const Value& name, const Value& type,
                               const Value& value) {
    Value x = Value::MakeMap();
    x["name"] = name;
    x["type"] = type;
    x["value"] = value;
    x["id"] = id;
    auto iter = Nvti._map().find(knowntext::kNVTI_preference);
    Value newVal = Value::MakeArray();
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
        LOG_WARNING("preference array not exist " + Nvti[knowntext::kNVTI_filename].ToString());
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
    Value newVal = Value::MakeMap();
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
    Value newVal = Value::MakeMap();
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
        portList = Env[knowntext::kENV_opened_tcp].text;
    } else {
        portList = Env[knowntext::kENV_opened_udp].text;
    }
    std::string key = "," + port.ToString();
    if (portList.find(key) != std::string::npos) {
        return true;
    }
    //Services/wwww
    Value ret = Storage->GetItem(port.ToString(), false);
    return !ret.IsNULL();
}

Value OVAContext::GetKbItem(const std::string& name) {
    if (IsForkedTask) {
        return Fork.Snapshot->GetItem(name, Fork.GetItemPos(name));
    }
    size_t Count = Storage->GetItemSize(name);
    if (Count > 1) {
        Fork.Names.push_back(name);
        Fork.Values.push_back((int)Count);
        Fork.Current.push_back(0);
        return Storage->GetItem(name, 0);
    }
    return Storage->GetItem(name, -1);
}

void OVAContext::SetKbItem(const std::string& name, const Value& val) {
    if (IsForkedTask) {
        if (Fork.GetItemPos(name) != -1) {
            LOG_ERROR("In forked task update a forking key: " + name);
        }
    }
    return Storage->SetItem(name, val);
}