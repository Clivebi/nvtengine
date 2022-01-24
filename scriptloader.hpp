#pragma once
#include "engine/vm.hpp"
class ScriptLoaderImplement : public Interpreter::ScriptLoader {
private:
    Interpreter::ScriptLoader* mLoader;
    Interpreter::ScriptLoader* mBuiltinLoader;

public:
    ScriptLoaderImplement(Interpreter::ScriptLoader* loader,
                          Interpreter::ScriptLoader* builtinLoader)
            : mLoader(loader), mBuiltinLoader(builtinLoader) {}
    scoped_refptr<Interpreter::Script> LoadScript(const std::string& name, std::string& error) {
        if (name == "nasl.sc" || name == "servicedetect.sc") {
            return mBuiltinLoader->LoadScript(name, error);
        }
        return mLoader->LoadScript(name, error);
    }
};