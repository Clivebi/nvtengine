#pragma once
#include "../../engine/check.hpp"
#include "../../engine/vm.hpp"
#include "./support/scriptstorage.hpp"
#include "knowntext.hpp"
using namespace Interpreter;
struct ForkedValue {
    std::vector<std::string> Names;
    std::vector<int> Values;
    std::vector<int> Current;
    scoped_refptr<support::ScriptStorage> Snapshot;

    bool IsForked() const { return Names.size() > 0; }

    bool IsNameExist(const std::string& name) const{
        for (auto n : Names) {
            if (n == name) {
                return true;
            }
        }
        return false;
    }

    int GetItemPos(const std::string& name) const{
        for (size_t i = 0; i < Names.size(); i++) {
            if (name == Names[i]) {
                return Current[i];
            }
        }
        return -1;
    }
    bool UpdateValue(size_t i) {
        int nValue = Current[i] + 1;
        if (nValue >= Values[i]) {
            if (i == 0) {
                return false;
            }
            if (!UpdateValue(i - 1)) {
                return false;
            }
            nValue = 0;
        }
        Current[i] = nValue;
        return true;
    }

    bool PrepareForNextScriptExecute() { return UpdateValue(Current.size() - 1); }
};

struct OVAContext {
    Value Nvti;
    const Value Prefs;
    const Value Env;
    bool IsForkedTask;
    std::string Host;
    ForkedValue Fork;
    const std::string ScriptFileName;
    scoped_refptr<support::ScriptStorage> Storage;

    explicit OVAContext(std::string script, const Value& pref, const Value& env,
                        scoped_refptr<support::ScriptStorage> storage)
            : ScriptFileName(script),
              Prefs(pref),
              Env(env),
              Storage(storage),
              IsForkedTask(false),
              Host(""),
              Fork() {
        Nvti = Value::MakeMap();
        Nvti.SetValue(knowntext::kNVTI_filename, script);
    }

    std::string NvtiString() { return Nvti.ToJSONString(false); }
    //for script preference
    void AddPreference(const Value& id, const Value& name, const Value& type, const Value& value);
    Value GetPreference(const Value& id, const Value& name);
    void GetPreferenceFile(const Value& id, const Value& name, std::string& content);
    void AddXref(const Value& name, const Value& value);
    void AddTag(const Value& name, const Value& value);

    bool IsPortInOpenedRange(Value& port, bool tcp);

    Value GetKbItem(const std::string& name);
    void SetKbItem(const std::string& name, const Value& val);
};

inline OVAContext* GetOVAContext(Executor* vm) {
    return (struct OVAContext*)vm->GetUserContext();
}
