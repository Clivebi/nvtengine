#include <dirent.h>
#include <stdio.h>

#include <fstream>

#include "./modules/openvas/api.hpp"
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"

char* read_file_content(const char* path, int* file_size) {
    FILE* f = NULL;
    char* content = NULL;
    size_t size = 0, read_size = 0;
    f = fopen(path, "r");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = (char*)malloc(size + 2);
    if (content == NULL) {
        fclose(f);
        return NULL;
    }
    read_size = fread(content, 1, size, f);
    fclose(f);
    if (read_size != size) {
        free(content);
        return NULL;
    }
    *file_size = (read_size + 2);
    content[read_size] = 0;
    content[read_size + 1] = 0;
    return content;
}

void ReadDir(std::string path, std::list<std::string>& result) {
    struct dirent* entry = NULL;
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type & DT_DIR) {
            continue;
        }
        if (entry->d_namlen > 0) {
            std::string full = "";
            full.append(entry->d_name, entry->d_namlen);
            if (full.find(".sc") != std::string::npos && full.find(".inc.") == std::string::npos) {
                result.push_back(full);
            }
        }
    }
    closedir(dir);
}

bool stop = false;
class DefaultExecutorCallback : public Interpreter::ExecutorCallback {
protected:
    std::string mFolder;

public:
    DefaultExecutorCallback(std::string folder) : mFolder(folder) {}

    void OnScriptWillExecute(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                             Interpreter::VMContext* ctx) {
        vm->RequireScript("nasl.sc", ctx);
    }
    void OnScriptExecuted(Interpreter::Executor* vm, scoped_refptr<Interpreter::Script> Script,
                          Interpreter::VMContext* ctx) {}
    void* LoadScriptFile(Interpreter::Executor* vm, const char* name, size_t& size) {
        std::string path = mFolder + name;
        if (std::string(name) == "nasl.sc") {
            path = "../script/nasl.sc";
        }
        int iSize = 0;
        void* ptr = read_file_content(path.c_str(), &iSize);
        size = iSize;
        return ptr;
    }
    void OnScriptError(Interpreter::Executor* vm, const char* name, const char* msg) {
        std::string error = msg;
        if (error.find("syntax error") != std::string::npos) {
            stop = true;
        }
        if (error.find("function not found") != std::string::npos) {
            return;
        }
        printf("%s %s\n", name, msg);
    }
};

void UpdateNVTI() {
    std::list<std::string> result;
    ReadDir("/Volumes/work/convert", result);
    Value ret = Value::make_array();
    auto iter = result.begin();
    while (iter != result.end()) {
        OVAContext context(*iter);
        DefaultExecutorCallback callback("/Volumes/work/convert/");
        Interpreter::Executor exe(&callback, &context);
        RegisgerModulesBuiltinMethod(&exe);
        bool error = exe.Execute(iter->c_str(), false);
        if (error) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                openvas::NVTIDataBase db("attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        iter++;
    }
    {
        openvas::NVTIDataBase db("attributes.db");
        db.UpdateAll(ret);
        ret._array().clear();
    }
}

bool ExecuteFile(std::string path, std::string name) {
    OVAContext context(name);
    DefaultExecutorCallback callback(path);
    Interpreter::Executor exe(&callback, &context);
    RegisgerModulesBuiltinMethod(&exe);
    bool ret = exe.Execute(name.c_str(), false);
    if (ret) {
        openvas::NVTIDataBase* db = new openvas::NVTIDataBase("attributes.db");
        db->UpdateOne(context.Nvti);
        Value old = db->Get("1.3.6.1.4.1.25623.1.0.103680");
        Value fromName = db->GetFromFileName(name);
        assert(old == fromName);
    }
    std::cout << context.NvtiString() << std::endl;
    return ret;
}

void test_execute_full(bool debug) {
    std::list<std::string> result;
    ReadDir("/Volumes/work/convert", result);
    auto iter = result.begin();
    while (iter != result.end()) {
        if (debug) {
            std::cout << "start execute " << *iter << std::endl;
        }
        ExecuteFile("/Volumes/work/convert/", *iter);
        if (stop) {
            break;
        }
        iter++;
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string folder = argv[1];
        size_t i = folder.rfind('/');
        std::string dir = folder.substr(0, i + 1);
        std::string name = folder.substr(i + 1);
        ExecuteFile(dir, name);
        std::cout << Interpreter::Status::ToString() << std::endl;
        return 0;
    }
    UpdateNVTI();
    std::cout << Interpreter::Status::ToString() << std::endl;
    return 0;
}