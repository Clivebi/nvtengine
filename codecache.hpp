#pragma once
#include "blockfile.hpp"
#include "engine/vm.hpp"
#include "taskmgr.hpp"

class CodeCacheWriter : public Interpreter::ScriptCache {
private:
    BlockFile* mStorage;

public:
    explicit CodeCacheWriter(FilePath path) {
        mStorage = BlockFile::NewWriter(path);
        ScriptCacheImplement::Shared();
    }
    ~CodeCacheWriter() { delete mStorage; }
    bool OnNewScript(scoped_refptr<Script> Script) {
        if (!mStorage->IsFileExist(Script->Name)) {
            std::stringstream o;
            Script->WriteToStream(o);
            std::string res = o.str();
            mStorage->Write(Script->Name, res.c_str(), res.size());
        }
        return ScriptCacheImplement::Shared()->OnNewScript(Script);
    }
    scoped_refptr<const Script> GetCachedScript(const std::string& name) {
        return ScriptCacheImplement::Shared()->GetCachedScript(name);
    }
};