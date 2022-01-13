
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
            if (pos < 0 || (unsigned int)pos >= iter->second.size()) {
                return iter->second.back();
            }
            return iter->second[pos];
        }
        return Value();
    }

    size_t GetItemSize(const std::string& name) {
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

    Value GetItemKeys(const std::string& partten) {
        std::vector<Value> keys;
        for (auto iter : mValues) {
            if (Interpreter::IsMatchString(iter.first, partten)) {
                keys.push_back(iter.first);
            }
        }
        return keys;
    }

    Value GetItemList(const std::string& item) {
        auto iter = mValues.find(item);
        if (iter != mValues.end()) {
            return iter->second;
        }
        return Value();
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
