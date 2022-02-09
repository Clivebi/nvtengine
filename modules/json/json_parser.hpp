#pragma once
#include <list>
#include <map>
#include <string>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif

namespace json {

class JSONValue {
public:
    enum {
        NUMBER,
        BOOL,
        NIL,
        STRING,
        OBJECT,
        ARRAY,
    };
    ~JSONValue() {}

public:
    int Type;
    std::string Value;
    std::map<std::string, JSONValue*> Object;
    std::list<JSONValue*> Array;
    JSONValue() : Array(), Object(), Value(""), Type(NIL) {}
};

class JSONMember {
public:
    std::string Member;
    JSONValue* Value;
    JSONMember() : Member(""), Value(NULL) {}
};

class JSONParser {
private:
    std::list<std::string*> mStringHolder;
    std::string mScanningString;
    std::list<JSONValue*> mValueHolder;
    std::list<JSONMember*> mMemberHolder;
    void* mUserContext;

public:
    JSONParser(void* Context)
            : root(NULL),
              mUserContext(Context),
              mStringHolder(),
              mScanningString(),
              mValueHolder(),
              mMemberHolder() {}
    ~JSONParser() {
        auto iter = mStringHolder.begin();
        while (iter != mStringHolder.end()) {
            delete (*iter);
            iter++;
        }
        auto iter2 = mValueHolder.begin();
        while (iter2 != mValueHolder.end()) {
            delete (*iter2);
            iter2++;
        }
        auto iter3 = mMemberHolder.begin();
        while (iter3 != mMemberHolder.end()) {
            delete (*iter3);
            iter3++;
        }
        root = NULL;
    }
    JSONValue* root;

    void* GetContext() { return mUserContext; }

public:
    void StartScanningString() { mScanningString.clear(); }
    void AppendToScanningString(char ch) { mScanningString += ch; }
    void AppendToScanningString(const char* text) { mScanningString += text; }
    const char* FinishScanningString() { return NewString(mScanningString.c_str()); }
    const char* NewString(const char* src) {
        std::string* ptr = new std::string(src);
        mStringHolder.push_back(ptr);
        return ptr->c_str();
    }

    JSONValue* NewValue(const char* value) {
        JSONValue* ptr = new JSONValue();
        ptr->Type = JSONValue::STRING;
        ptr->Value = value;
        mValueHolder.push_back(ptr);
        return ptr;
    }

    JSONValue* NewArray(JSONValue* ele) {
        JSONValue* ptr = new JSONValue();
        ptr->Type = JSONValue::ARRAY;
        ptr->Array.push_back(ele);
        mValueHolder.push_back(ptr);
        return ptr;
    }
    JSONValue* AddToArray(JSONValue* array, JSONValue* ele) {
        array->Array.push_back(ele);
        return array;
    }
    JSONValue* NewObject(JSONMember* member) {
        JSONValue* ptr = new JSONValue();
        ptr->Type = JSONValue::OBJECT;
        ptr->Object[member->Member] = member->Value;
        mValueHolder.push_back(ptr);
        return ptr;
    }
    JSONMember* NewMember(const char* Member, JSONValue* val) {
        JSONMember* memebr = new JSONMember();
        memebr->Member = Member;
        memebr->Value = val;
        mMemberHolder.push_back(memebr);
        return memebr;
    }
    JSONValue* AddMember(JSONValue* obj, JSONMember* member) {
        obj->Object[member->Member] = member->Value;
        return obj;
    }
};

} // namespace json
