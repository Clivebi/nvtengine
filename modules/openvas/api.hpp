#pragma once
#include "../../engine/vm.hpp"
#include "../check.hpp"
#include "./support/scriptstorage.hpp"
#include "knowntext.hpp"
using namespace Interpreter;
struct OVAContext {
    const std::string ScriptFileName;
    std::string Host;
    Value Nvti;
    const Value& Prefs;
    const Value& Env;
    scoped_refptr<support::ScriptStorage> Storage;
    explicit OVAContext(std::string script, const Value& pref, const Value& env,
                        scoped_refptr<support::ScriptStorage> storage)
            : ScriptFileName(script), Prefs(pref), Env(env), Storage(storage) {
        Nvti = Value::make_map();
        Nvti[knowntext::kNVTI_filename] = script;
    }
    std::string NvtiString() { return Nvti.ToJSONString(false); }
    //for script preference
    void AddPreference(const Value& id, const Value& name, const Value& type, const Value& value);
    Value GetPreference(const Value& id, const Value& name);
    void GetPreferenceFile(const Value& id, const Value& name, std::string& content);
    void AddXref(const Value& name, const Value& value);
    void AddTag(const Value& name, const Value& value);

    bool IsPortInOpenedRange(Value& port, bool tcp);
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
