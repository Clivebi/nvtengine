#pragma once
#include "engine/vm.hpp"
class ScriptLoaderImplement : public Interpreter::ScriptLoader {
private:
    Interpreter::ScriptLoader* mLoader;
    Interpreter::ScriptLoader* mBuiltinLoader;
    std::map<std::string, int> mBuiltinScriptFiles;

public:
    ScriptLoaderImplement(Interpreter::ScriptLoader* loader,
                          Interpreter::ScriptLoader* builtinLoader)
            : mLoader(loader), mBuiltinLoader(builtinLoader) {}
    scoped_refptr<Interpreter::Script> LoadScript(const std::string& name, std::string& error) {
        if (IsBuiltinScript(name)) {
            return mBuiltinLoader->LoadScript(name, error);
        }
        return mLoader->LoadScript(name, error);
    }
    void AddBuiltinScriptFiles(std::list<std::string>& files) {
        for (auto iter : files) {
            mBuiltinScriptFiles[iter] = 1;
        }
    }
    bool IsBuiltinScript(const std::string& name) {
        return mBuiltinScriptFiles.find(name) != mBuiltinScriptFiles.end();
    }
};