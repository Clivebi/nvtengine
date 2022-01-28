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
#include "scriptloader.hpp"

class InterpreterExecutorCallback : public Interpreter::ExecutorCallback {
public:
    InterpreterExecutorCallback() {}

    void OnScriptWillExecute(Interpreter::Executor* vm,
                             scoped_refptr<const Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {}
    void OnScriptExecuted(Interpreter::Executor* vm,
                          scoped_refptr<const Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script, VMContext* ctx) {}
    void OnScriptError(Interpreter::Executor* vm, const std::string& name, const std::string& msg) {
        std::string error = msg;
        LOG_ERROR(std::string(name) + " " + msg);
    }
};

bool ExecuteScript(FilePath path) {
    std::string dir = path.dir();
    std::string name = path.base_name();
    StdFileIO baseIO(dir);
    StdFileIO builtinIO("../script/");
    DefaultScriptLoader baseLoader(&baseIO, false);
    DefaultScriptLoader builtinLoader(&builtinIO, false);
    ScriptLoaderImplement loader(&baseLoader,&builtinLoader);
    OVAContext context(name, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
    InterpreterExecutorCallback callback;
    Interpreter::Executor Engine(&callback, &loader);
    Engine.SetUserContext(&context);
    RegisgerModulesBuiltinMethod(&Engine);
    return Engine.Execute(name.c_str(), 5 * 60, true);
}

void StreamTest() {
    unsigned char buf[9];
    long long llv;
    varint::encode(0xFFFFFFFFFFF, buf);
    varint::decode(buf, llv);

    StdFileIO IO("../test");
    DefaultScriptLoader loader(&IO, false);
    std::string error;
    scoped_refptr<Script> script = loader.LoadScript("test.sc", error);
    std::stringstream o;
    script->WriteToStream(o);
    std::string src = o.str();
    size_t n = 64;
    size_t i = 0;
    std::string result = "";
    while (i < n) {
        std::string hex = "";
        std::string str = "";
        char buffer[6] = {0};
        for (int j = 0; j < 16; j++, i++) {
            if (i < n) {
#ifdef _WIN32
                sprintf_s(buffer, 6, "%02X ", (BYTE)src[i]);
#else
                snprintf(buffer, 6, "%02X ", (BYTE)src[i]);
#endif
                hex += buffer;
                if (isprint(src[i])) {
                    str += (char)src[i];
                } else {
                    str += ".";
                }
            } else {
                hex += "   ";
                str += " ";
            }
        }
        result += hex;
        result += "\t";
        result += str;
        result += "\n";
    }
    std::cout << result;

    scoped_refptr<Script> second = new Script("");
    second->ReadFromStream(o);
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