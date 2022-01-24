#pragma once
#include "blockfile.hpp"
#include "engine/vm.hpp"
#include "taskmgr.hpp"

class CodeCacheWriter : public Interpreter::ScriptCache {
private:
    BlockFile* mStorage;
    ScriptCacheImplement mCache;

public:
    explicit CodeCacheWriter(FilePath path) { mStorage = BlockFile::NewWriter(path); }
    ~CodeCacheWriter() {
        delete mStorage;
    }
    bool OnNewScript(scoped_refptr<Script> Script) {
        if (!mStorage->IsFileExist(Script->Name)) {
            std::stringstream o;
            Script->WriteToStream(o);
            std::string res = o.str();
            mStorage->Write(Script->Name, res.c_str(), res.size());
        }
        return mCache.OnNewScript(Script);
    }
    scoped_refptr<const Script> GetCachedScript(const std::string& name) {
        return mCache.GetCachedScript(name);
    }
};