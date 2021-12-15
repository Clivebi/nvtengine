
#pragma once
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "../../../engine/value.hpp"
using namespace Interpreter;
namespace support {
class ScriptStorage : public CRefCountedThreadSafe<ScriptStorage> {
protected:
    std::map<std::string, std::vector<Value>> mValues;

public:
    explicit ScriptStorage() : mValues() {}
    Value GetItem(const std::string& name, int pos) {
        auto iter = mValues.find(name);
        if (iter != mValues.end()) {
            if (pos < 0 || pos >= iter->second.size()) {
                return iter->second.back();
            }
            return iter->second[pos];
        }
        return Value();
    }

    int GetItemSize(const std::string& name) {
        auto iter = mValues.find(name);
        if (iter != mValues.end()) {
            return iter->second.size();
        }
        return 0;
    }

    void ReplaceItem(const std::string& name, const Value& val) {
        mValues[name].clear();
        mValues[name].push_back(val);
    }

    Value GetItemList(const std::string& partten) {
        if (partten.find("*") == std::string::npos) {
            auto iter = mValues.find(partten);
            if (iter != mValues.end()) {
                return iter->second;
            }
            return Value();
        }
        Value ret = Value::make_map();
        auto iter = mValues.begin();
        while (iter != mValues.end()) {
            if (Interpreter::IsMatchString(iter->first, partten)) {
                auto iter2 = iter->second.begin();
                if (iter->second.size() > 1) {
                    LOG("Check Key", iter->first, " this have more than one value");
                }
                while (iter2 != iter->second.end()) {
                    ret[iter->first] = (*iter2);
                    iter2++;
                }
            }
            iter++;
        }
        if (ret.Length() == 0) {
            return Value();
        }
        return ret;
    }

    void SetItem(const std::string& name, const Value& val) {
        //std::cout << name << "\t" << val.ToString() << std::endl;
        if (IsExist(mValues[name], val)) {
            return;
        }
        mValues[name].push_back(val);
    }

    scoped_refptr<ScriptStorage> Clone() {
        scoped_refptr<ScriptStorage> New = new ScriptStorage();
        for (auto v : mValues) {
            std::vector<Value> newVector;
            for (auto k : v.second) {
                newVector.push_back(k.Clone());
            }
            New->mValues[v.first] = newVector;
        }
        return New;
    }

    void Combine(const ScriptStorage* from) {
        for (auto v : from->mValues) {
            for (auto k : v.second) {
                SetItem(v.first, k);
            }
        }
    }

    void AddService(const std::string& name, int port) { SetItem("Services/" + name, port); }
    Value GetService(const std::string& name) { return GetItem("Services/" + name, true); }

private:
    bool IsExist(std::vector<Value>& group, Value val) {
        auto iter = group.begin();
        while (iter != group.end()) {
            if (*iter == val) {
                return true;
            }
            iter++;
        }
        return false;
    }
    DISALLOW_COPY_AND_ASSIGN(ScriptStorage);
};
} // namespace support
