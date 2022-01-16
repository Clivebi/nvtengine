#include <dirent.h>
#include <stdio.h>

#include <fstream>
extern "C" {
#include "thirdpart/masscan/hostscan.h"
}
#include "./modules/openvas/api.hpp"
#include "engine/vm.hpp"
#include "fileio.hpp"
#include "filepath.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"

class InterpreterExecutorCallback : public Interpreter::ExecutorCallback {
protected:
    StdFileIO IO;

public:
    InterpreterExecutorCallback(FilePath folder) : IO(folder) {}

    void OnScriptWillExecute(Interpreter::Executor* vm,
                             scoped_refptr<const Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {}
    void OnScriptExecuted(Interpreter::Executor* vm,
                          scoped_refptr<const Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script, VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        void* ptr = IO.Read(name, size);
        return ptr;
    }
    void OnScriptError(Interpreter::Executor* vm, const char* name, const char* msg) {
        std::string error = msg;
        LOG_ERROR(std::string(name) + " " + msg);
    }
};

bool ExecuteScript(FilePath path) {
    std::string dir = path.dir();
    std::string name = path.base_name();
    OVAContext context(name, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
    InterpreterExecutorCallback callback(dir);
    Interpreter::Executor Engine(&callback, &context);
    RegisgerModulesBuiltinMethod(&Engine);
    return Engine.Execute(name.c_str(), 5 * 60, true);
}

int main(int argc, char* argv[]) {
    masscan_init();
    InitializeLibray();
    g_LogLevel = LEVEL_DEBUG;
    if (argc > 1) {
        ExecuteScript(argv[1]);
        std::cout << Interpreter::Status::ToString() << std::endl;
        return 0;
    }
    return -1;
}