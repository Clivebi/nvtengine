
#pragma once
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "../../../engine/value.hpp"
using namespace Interpreter;
namespace openvas {
class ScriptStorage : public CRefCountedThreadSafe<ScriptStorage> {
protected:
    std::map<std::string, std::vector<Value>> mValues;

public:
    Value GetItem(const std::string& name, bool onlylast) {
        auto iter = mValues.find(name);
        if (iter != mValues.end()) {
            if (onlylast) {
                return iter->second.back();
            }
            return iter->second;
        }
        return Value();
    }

    void ReplaceItem(const std::string& name, const Value& val) {
        mValues[name].clear();
        mValues[name].push_back(val);
    }

    Value GetItemList(const std::string& partten) {
        if (partten.find("*") == std::string::npos) {
            return GetItem(partten, false);
        }
        Value ret = Value::make_array();
        auto iter = mValues.begin();
        while (iter != mValues.end()) {
            if (Interpreter::IsMatchString(iter->first, partten)) {
                auto iter2 = iter->second.begin();
                while (iter2 != iter->second.end()) {
                    ret._array().push_back(*iter2);
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
};
} // namespace openvas
