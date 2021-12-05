#include <dirent.h>
#include <stdio.h>

#include <fstream>

#include "./modules/openvas/api.hpp"
#include "engine/vm.hpp"
#include "fileio.hpp"
#include "filepath.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"

class InterpreterExecutorCallback : public Interpreter::ExecutorCallback {
protected:
    FilePath mFolder;

public:
    InterpreterExecutorCallback(FilePath folder) : mFolder(folder) {}

    void OnScriptWillExecute(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {}
    void OnScriptExecuted(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = (mFolder + FilePath(name));
        FileIO io;
        void* ptr = io.Read(path, size);
        return ptr;
    }
    void OnScriptError(Interpreter::Executor* vm, const char* name, const char* msg) {
        std::string error = msg;
        LOG(std::string(name) + " " + msg);
    }
};

bool ExecuteScript(std::string path) {
    size_t i = path.rfind('/');
    std::string dir = path.substr(0, i + 1);
    std::string name = path.substr(i + 1);
    OVAContext context(name,Value::make_map(),Value::make_map(),new support::ScriptStorage());
    InterpreterExecutorCallback callback(dir);
    Interpreter::Executor Engine(&callback, &context);
    RegisgerModulesBuiltinMethod(&Engine);
    return Engine.Execute(name.c_str(), true);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        return ExecuteScript(argv[1]);
    }
    return -1;
}