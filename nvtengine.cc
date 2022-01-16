#include <dirent.h>
#include <stdio.h>
#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

#include <fstream>
#include <regex>
extern "C" {
#include "thirdpart/masscan/hostscan.h"
}
#include "./modules/openvas/api.hpp"
#include "./modules/openvas/support/sconfdb.hpp"
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"
#include "ntvpref.hpp"
#include "parseargs.hpp"
#include "taskmgr.hpp"
#include "testoids.hpp"
#include "vfsfileio.hpp"

#define PRODUCT_NAME "NVTStudio"

void CollectAllScript(FilePath path, FilePath relative_path, std::list<std::string>& result) {
    struct dirent* entry = NULL;
    DIR* dir = opendir(std::string(path).c_str());
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type & DT_DIR) {
            if (entry->d_name[0] != '.') {
                CollectAllScript(path + entry->d_name, relative_path + entry->d_name, result);
            }
            continue;
        }
        if (entry->d_namlen > 0) {
            FilePath short_path = relative_path;
            short_path += entry->d_name;
            std::string element = short_path;
            if (element.find(".sc") != std::string::npos &&
                element.find(".inc.") == std::string::npos) {
                result.push_back(element);
            }
        }
    }
    closedir(dir);
}

void UpdateNVTIFromLocalFS(std::string Folder, std::string home) {
    std::list<std::string> result;
    StdFileIO IO(Folder);
    CollectAllScript(Folder, "", result);
    Value ret = Value::MakeArray();
    for (auto iter : result) {
        OVAContext context(iter, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
        DefaultExecutorCallback callback("", &IO);
        callback.mDescription = true;
        Interpreter::Executor exe(&callback, &context);
        RegisgerModulesBuiltinMethod(&exe);
        bool ok = exe.Execute(iter.c_str(), false);
        if (ok) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                support::NVTIDataBase db(FilePath(home) + "attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        if (callback.mSyntaxError) {
            break;
        }
    }
    {
        support::NVTIDataBase db(FilePath(home) + "attributes.db");
        db.UpdateAll(ret);
        ret._array().clear();
    }
}

void UpdateNVTIFromVFS(std::string package, std::string home) {
    std::list<std::string> result;
    VFSFileIO IO(package);
    IO.EnumFile(result);
    Value ret = Value::MakeArray();
    for (auto iter : result) {
        OVAContext context(iter, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
        DefaultExecutorCallback callback("", &IO);
        callback.mDescription = true;
        Interpreter::Executor exe(&callback, &context);
        RegisgerModulesBuiltinMethod(&exe);
        bool ok = exe.Execute(iter.c_str(), false);
        if (ok) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                support::NVTIDataBase db(FilePath(home) + "attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        if (callback.mSyntaxError) {
            break;
        }
    }
    {
        support::NVTIDataBase db(FilePath(home) + "attributes.db");
        db.UpdateAll(ret);
        ret._array().clear();
    }
}

void UpdateNVTI(Interpreter::Value& pref) {
    NVTPref helper(pref);
    if (helper.script_folder().size() == 0 && helper.script_package().size() == 0) {
        std::cout << "You must provider script_folder or script_package in you config file"
                  << std::endl;
        return;
    }
    if (helper.script_package().size()) {
        UpdateNVTIFromVFS(helper.script_package(), helper.app_data_folder());
    } else {
        UpdateNVTIFromLocalFS(helper.script_folder(), helper.app_data_folder());
    }
}

void LoadOidList(ParseArgs& option, Interpreter::Value& pref, std::list<std::string>& result) {
    NVTPref helper(pref);
    //load from db
    if (option.GetOIDFilter().size()) {
        support::ScanConfig db(FilePath(helper.app_data_folder()) + "scanconfig.db");
        return db.Get(option.GetOIDFilter(), result);
    }
    //load from file
    {
        StdFileIO IO("");
        size_t size = 0;
        std::string rule;
        char* data = (char*)IO.Read("./rule.txt", size);
        if (data) {
            rule.assign(data, size);
            free(data);
            result = Interpreter::split(rule, ';');
            return;
        }
    }
    result = Interpreter::split(test_oids, ';');
}

void DoNVTTest(Interpreter::Value& pref, ParseArgs& option) {
    NVTPref helper(pref);
    if (helper.script_folder().size() == 0 && helper.script_package().size() == 0) {
        std::cout << "You must provider script_folder or script_package in you config file"
                  << std::endl;
        return;
    }
    if (option.GetHostList().size() == 0 || option.GetPortList().size() == 0) {
        std::cout << "You must provider target hosts and ports" << std::endl;
        return;
    }
    if (helper.script_package().size()) {
        std::list<std::string> oidList;
        LoadOidList(option, pref, oidList);
        VFSFileIO IO(helper.script_package());

        HostsTask task(option.GetHostList(), option.GetPortList(), pref, &IO);
        task.BeginTask(oidList, "10000");
        task.Join();
    } else {
        std::list<std::string> oidList;
        LoadOidList(option, pref, oidList);
        StdFileIO IO(helper.script_folder());

        HostsTask task(option.GetHostList(), option.GetPortList(), pref, &IO);
        task.BeginTask(oidList, "10000");
        task.Join();
    }
}

bool LoadJsonFromFile(std::string path, Interpreter::Value& pref) {
    StdFileIO IO("");
    size_t length = 0;
    void* data = IO.Read(std::string(path), length);
    if (data != NULL) {
        std::string text;
        text.assign((char*)data, length);
        pref = ParseJSON(text, true);
        free(data);
        return true;
    }
    return false;
}

bool LoadDefaultConfig(Interpreter::Value& pref) {
    std::string search = "";
#ifdef _WIN32
    search = getenv("AppData");
#error("please implement on windows")
    return false;
#endif
    char _default_path[][120] = {"/etc/", "/usr/local"};
    for (int i = 0; i < sizeof(_default_path) / sizeof(_default_path[0]); i++) {
        FilePath path = _default_path[i];
        path += PRODUCT_NAME;
        path += "nvtengine.conf";
        if (LoadJsonFromFile(path, pref)) {
            return true;
        }
    }
    return LoadJsonFromFile("./nvtengine.conf", pref);
}

int main(int argc, char* argv[]) {
#ifdef __APPLE__
    signal(SIGPIPE, SIG_IGN);
#endif
    masscan_init();
    ParseArgs options(argc, argv);
    std::string configFile = options.GetConfigFile();
    Interpreter::Value pref;
    if (configFile.size() == 0) {
        if (!LoadDefaultConfig(pref)) {
            options.PrintHelp();
            std::cout << " can't load default config file" << std::endl;
            return -1;
        }
    } else {
        if (!LoadJsonFromFile(configFile, pref)) {
            std::cout << " can't load config file" << std::endl;
            return -1;
        }
    }
    g_LogLevel = NVTPref(pref).log_level();
    if (options.IsHaveScanCommand()) {
        DoNVTTest(pref, options);
    } else if (options.IsHaveUpdateNVTDatabaseCommand()) {
        UpdateNVTI(pref);
    } else {
        options.PrintHelp();
        return -1;
    }
    return 0;
}