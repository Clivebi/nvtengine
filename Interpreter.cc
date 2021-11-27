#include <dirent.h>
#include <stdio.h>

#include <fstream>

#include "./modules/openvas/api.hpp"
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"
#include "taskmgr.hpp"

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

void TryLoadNVTI(std::string& name, Value& value) {
    openvas::NVTIDataBase db("attributes.db");
    value = db.GetFromFileName(name);
    std::cout << name << " : " << value.ToJSONString(false) << std::endl;
}

bool ExecuteFile(std::string path, std::string name) {
    OVAContext context(name);
    TryLoadNVTI(name, context.Nvti);
    DefaultExecutorCallback callback(path);
    Interpreter::Executor exe(&callback, &context);
    RegisgerModulesBuiltinMethod(&exe);
    bool ret = exe.Execute(name.c_str(), false);
    //std::cout << context.NvtiString() << std::endl;
    return ret;
}

int main(int argc, char* argv[]) {
    Value pref = Value::make_map();
    HostsTask host("", "", pref);
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