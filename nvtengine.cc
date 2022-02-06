#include <dirent.h>
#include <stdio.h>
#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

#include <fstream>
extern "C" {
#include "thirdpart/masscan/hostscan.h"
}
#include "./modules/openvas/api.hpp"
#include "./modules/openvas/support/sconfdb.hpp"
#include "codecache.hpp"
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"
#include "ntvpref.hpp"
#include "parseargs.hpp"
#include "scriptloader.hpp"
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
        FilePath short_path = relative_path;
        short_path += entry->d_name;
        std::string element = short_path;
        if (element.find(".sc") != std::string::npos) {
            result.push_back(element);
        }
    }
    closedir(dir);
}

void UpdateNVTIFromLocalFS(NVTPref& helper) {
    std::list<std::string> result;
    StdFileIO scriptIO(helper.script_folder());
    StdFileIO builtinIO(helper.builtin_script_path());
    DefaultScriptLoader scriptLoader(&scriptIO, false);
    DefaultScriptLoader builtinLoader(&builtinIO, false);
    ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);
    std::list<std::string> files;
    CollectAllScript(helper.builtin_script_path(), "", files);
    loader.AddBuiltinScriptFiles(files);
    CollectAllScript(helper.script_folder(), "", result);
    Value ret = Value::MakeArray();
    ScriptCache* cachePtr = NULL;
    ScriptCacheImplement cache;
    CodeCacheWriter cache2(FilePath(helper.app_data_folder()) + FilePath("code_cache.bin"));
    cachePtr = &cache;
    if (helper.enable_code_cache()) {
        cachePtr = &cache2;
    }
    DefaultExecutorCallback callback;
    callback.mDescription = true;
    Interpreter::Executor exe(&callback, &loader);
    RegisgerModulesBuiltinMethod(&exe);
    exe.SetScriptCacheProvider(cachePtr);
    for (auto iter : result) {
        if (iter.find(".inc.") != std::string::npos) {
            continue;
        }
        OVAContext context(iter, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
        exe.SetUserContext(&context);
        bool ok = exe.Execute(iter.c_str(), 5, helper.log_engine_warning());
        if (ok) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                support::NVTIDataBase db(FilePath(helper.app_data_folder()) + "attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        if (callback.mSyntaxError) {
            break;
        }
    }
    {
        support::NVTIDataBase db(FilePath(helper.app_data_folder()) + "attributes.db");
        db.UpdateAll(ret);
        ret._array().clear();
    }
    if (!helper.enable_code_cache()) {
        return;
    }
    //compire all left inc file
    for (auto iter : result) {
        if (iter.find(".inc.") == std::string::npos) {
            continue;
        }
        OVAContext context(iter, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
        exe.SetUserContext(&context);
        bool ok = exe.Execute(iter.c_str(), 5, helper.log_engine_warning(), true);
        if (callback.mSyntaxError) {
            break;
        }
    }
}

void UpdateNVTIFromVFS(NVTPref& helper) {
    std::list<std::string> result;
    VFSFileIO scriptIO(helper.script_package());
    StdFileIO builtinIO(helper.builtin_script_path());
    DefaultScriptLoader scriptLoader(&scriptIO, false);
    DefaultScriptLoader builtinLoader(&builtinIO, false);

    ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);

    std::list<std::string> files;
    CollectAllScript(helper.builtin_script_path(), "", files);
    loader.AddBuiltinScriptFiles(files);
    scriptIO.EnumFile(result);
    Value ret = Value::MakeArray();
    ScriptCache* cachePtr = NULL;
    ScriptCacheImplement cache;
    CodeCacheWriter cache2(FilePath(helper.app_data_folder()) + FilePath("code_cache.bin"));
    cachePtr = &cache;
    if (helper.enable_code_cache()) {
        cachePtr = &cache2;
    }
    DefaultExecutorCallback callback;
    callback.mDescription = true;
    Interpreter::Executor exe(&callback, &loader);
    RegisgerModulesBuiltinMethod(&exe);
    exe.SetScriptCacheProvider(cachePtr);
    for (auto iter : result) {
        OVAContext context(iter, Value::MakeMap(), Value::MakeMap(), new support::ScriptStorage());
        exe.SetUserContext(&context);
        bool ok = exe.Execute(iter.c_str(), 5, helper.log_engine_warning());
        if (ok) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                support::NVTIDataBase db(FilePath(helper.app_data_folder()) + "attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        if (callback.mSyntaxError) {
            break;
        }
    }
    {
        support::NVTIDataBase db(FilePath(helper.app_data_folder()) + "attributes.db");
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
        UpdateNVTIFromVFS(helper);
    } else {
        UpdateNVTIFromLocalFS(helper);
    }
}

void LoadOidList(ParseArgs& option, Interpreter::Value& pref, std::list<std::string>& result) {
    NVTPref helper(pref);
    //load from db
    if (option.GetOIDFilter().size()) {
        support::ScanConfig db(FilePath(helper.app_data_folder()) + "scanconfig.db");
        NVT_LOG_DEBUG("load oid list  from database with filter:", option.GetOIDFilter());
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
            NVT_LOG_DEBUG("load oid list from ./rule.txt");
            return;
        }
    }
    NVT_LOG_DEBUG("load oid list from builtin list");
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
    if (helper.enable_code_cache()) {
        std::list<std::string> oidList;
        LoadOidList(option, pref, oidList);
        BlockFile* block =
                BlockFile::NewReader((FilePath)helper.app_data_folder() + "code_cache.bin");
        if (block != NULL) {
            StdFileIO builtinIO(helper.builtin_script_path());
            DefaultScriptLoader scriptLoader(block, true);
            DefaultScriptLoader builtinLoader(&builtinIO, false);
            ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);
            std::list<std::string> files;
            CollectAllScript(helper.builtin_script_path(), "", files);
            loader.AddBuiltinScriptFiles(files);

            HostsTask task(option.GetHostList(), option.GetPortList(), pref, &loader);
            task.BeginTask(oidList, "10000");
            task.Join();
            delete block;
            return;
        }
    }
    if (helper.script_package().size()) {
        std::list<std::string> oidList;
        LoadOidList(option, pref, oidList);
        VFSFileIO scriptIO(helper.script_package());

        StdFileIO builtinIO(helper.builtin_script_path());
        std::list<std::string> files;
        CollectAllScript(helper.builtin_script_path(), "", files);
        DefaultScriptLoader scriptLoader(&scriptIO, false);
        DefaultScriptLoader builtinLoader(&builtinIO, false);
        ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);
        loader.AddBuiltinScriptFiles(files);

        HostsTask task(option.GetHostList(), option.GetPortList(), pref, &loader);
        task.BeginTask(oidList, "10000");
        task.Join();
    } else {
        std::list<std::string> oidList;
        LoadOidList(option, pref, oidList);
        StdFileIO scriptIO(helper.script_folder());
        StdFileIO builtinIO(helper.builtin_script_path());
        std::list<std::string> files;
        CollectAllScript(helper.builtin_script_path(), "", files);
        DefaultScriptLoader scriptLoader(&scriptIO, false);
        DefaultScriptLoader builtinLoader(&builtinIO, false);
        ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);
        loader.AddBuiltinScriptFiles(files);

        HostsTask task(option.GetHostList(), option.GetPortList(), pref, &loader);
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
        NVT_LOG_DEBUG("user config on the:", path);
        NVT_LOG_DEBUG("Preferences:", pref.ToJSONString());
        return true;
    }
    return false;
}

bool LoadDefaultConfig(Interpreter::Value& pref) {
    std::string search = "";
#ifdef _WIN32
    search = getenv("AppData");
    FilePath path = search;
    path += PRODUCT_NAME;
    path += "nvtengine.conf";
    if (LoadJsonFromFile(path, pref)) {
        return true;
    }
    if (LoadJsonFromFile(".\\etc\\nvtengine.conf", pref)) {
        return true;
    }

    if (LoadJsonFromFile("..\\etc\\nvtengine.conf", pref)) {
        return true;
    }
    return false;
#else
    char _default_path[][120] = {"/etc/", "/usr/local"};
    for (int i = 0; i < sizeof(_default_path) / sizeof(_default_path[0]); i++) {
        FilePath path = _default_path[i];
        path += PRODUCT_NAME;
        path += "nvtengine.conf";
        if (LoadJsonFromFile(path, pref)) {
            return true;
        }
    }
    if (LoadJsonFromFile("./nvtengine.conf", pref)) {
        return true;
    }
    if (LoadJsonFromFile("./etc/nvtengine.conf", pref)) {
        return true;
    }
    if (LoadJsonFromFile("../etc/nvtengine.conf", pref)) {
        return true;
    }
    return false;
#endif
}

int main(int argc, char* argv[]) {
#ifdef __APPLE__
    signal(SIGPIPE, SIG_IGN);
#endif
    masscan_init();
    InitializeLibray();
    ParseArgs options(argc, argv);
    if (!options.IsHaveScanCommand() && !options.IsHaveUpdateNVTDatabaseCommand()) {
        options.PrintHelp();
        return -1;
    }
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
        std::cout << "do scan target command" << std::endl;
        DoNVTTest(pref, options);
    } else if (options.IsHaveUpdateNVTDatabaseCommand()) {
        std::cout << "do update nvti database command" << std::endl;
        UpdateNVTI(pref);
    } else {
        options.PrintHelp();
        return -1;
    }
    std::cout << Interpreter::Status::ToString() << std::endl;
    return 0;
}