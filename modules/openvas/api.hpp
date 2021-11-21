#pragma once
#include "../../engine/vm.hpp"
#include "../check.hpp"
#include "./support/scriptstorage.hpp"
using namespace Interpreter;
struct OVAContext {
    std::string Name;
    Value Nvti;
    Value Prefs; // for global preference
    scoped_refptr<openvas::ScriptStorage> Storage;
    explicit OVAContext(std::string name) : Name(name) {
        Nvti = Value::make_map();
        Nvti["filename"] = name;
        Storage = new openvas::ScriptStorage();
        Prefs = Value::make_map();
    }
    std::string NvtiString() { return Nvti.ToJSONString(); }
    //for script preference
    void AddPreference(const Value& id, const Value& name, const Value& type, const Value& value);
    Value GetPreference(const Value& id, const Value& name);
    void GetPreferenceFile(const Value& id, const Value& name, std::string& content);
    void AddXref(const Value& name, const Value& value);
    void AddTag(const Value& name, const Value& value);
};

inline OVAContext* GetOVAContext(Executor* vm) {
    return (struct OVAContext*)vm->GetUserContext();
}
