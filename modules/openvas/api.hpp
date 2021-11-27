#pragma once
#include "../../engine/vm.hpp"
#include "../check.hpp"
#include "./support/scriptstorage.hpp"
using namespace Interpreter;
struct OVAContext {
    std::string Name;
    std::string Host; // current runing host
    Value Nvti;
    Value Prefs; // for global preference
    Value Env;
    scoped_refptr<openvas::ScriptStorage> Storage;
    explicit OVAContext(std::string name) : Name(name) {
        Nvti = Value::make_map();
        Nvti["filename"] = name;
        Storage = new openvas::ScriptStorage();
        Prefs = Value::make_map();
        Env = Value::make_map();
        InitEnv();
    }
    std::string NvtiString() { return Nvti.ToJSONString(false); }
    //for script preference
    void AddPreference(const Value& id, const Value& name, const Value& type, const Value& value);
    Value GetPreference(const Value& id, const Value& name);
    void GetPreferenceFile(const Value& id, const Value& name, std::string& content);
    void AddXref(const Value& name, const Value& value);
    void AddTag(const Value& name, const Value& value);
    void InitEnv();
};

inline int GetInt(std::vector<Value>& args, int pos, int default_value) {
    if (pos < args.size()) {
        if (args[pos].IsInteger()) {
            return args[pos].Integer;
        }
    }
    return default_value;
}

inline OVAContext* GetOVAContext(Executor* vm) {
    return (struct OVAContext*)vm->GetUserContext();
}
