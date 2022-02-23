#include <dirent.h>
#include <stdio.h>
#ifdef _WIN32
#else
#include <unistd.h>
#endif // _WIN32

#include <fstream>
extern "C" {
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "thirdpart/masscan/hostscan.h"
}
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "./modules/net/socket.hpp"
#include "./modules/openvas/api.hpp"
#include "./modules/openvas/support/sconfdb.hpp"
#include "codecache.hpp"
#include "engine/vm.hpp"
#include "jsonrpc.hpp"
#include "manger.hpp"
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
    ScriptCache* cachePtr = ScriptCacheImplement::Shared();
    CodeCacheWriter cache2(FilePath(helper.app_data_folder()) + FilePath("code_cache.bin"));
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
    ScriptCache* cachePtr = ScriptCacheImplement::Shared();
    CodeCacheWriter cache2(FilePath(helper.app_data_folder()) + FilePath("code_cache.bin"));
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
    if (option.GetOption("filter").size()) {
        support::ScanConfig db(FilePath(helper.app_data_folder()) + "scanconfig.db");
        NVT_LOG_DEBUG("load oid list  from database with filter:", option.GetOption("filter"));
        return db.Get(option.GetOption("filter"), result);
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

FileIO* CreateBuiltinFileIO(NVTPref& helper) {
    return new StdFileIO(helper.builtin_script_path());
}

std::list<std::string> GetBuiltinFileList(NVTPref& helper) {
    std::list<std::string> files;
    CollectAllScript(helper.builtin_script_path(), "", files);
    return files;
}

FileIO* CreateFileIO(NVTPref& helper, bool& Encoded) {
    BlockFile* block = NULL;
    if (helper.enable_code_cache()) {
        block = BlockFile::NewReader((FilePath)helper.app_data_folder() + "code_cache.bin");
    }
    if (block != NULL) {
        Encoded = true;
        return block;
    }
    if (helper.script_package().size()) {
        return new VFSFileIO(helper.script_package());
    }
    if (helper.script_folder().size()) {
        return new StdFileIO(helper.script_folder());
    }
    return NULL;
}

void ServeDaemon(ParseArgs& options, Interpreter::Value& pref) {
    NVTPref helper(pref);
    bool Encode = false;
    FileIO* pBuiltinFileIO = CreateBuiltinFileIO(helper);
    FileIO* pFileIO = CreateFileIO(helper, Encode);
    if (pBuiltinFileIO == NULL || pFileIO == NULL) {
        NVT_LOG_ERROR("Init FileIO Failed");
        return;
    }
    DefaultScriptLoader scriptLoader(pFileIO, Encode);
    DefaultScriptLoader builtinLoader(pBuiltinFileIO, false);
    ScriptLoaderImplement loader(&scriptLoader, &builtinLoader);
    std::list<std::string> list = GetBuiltinFileList(helper);
    loader.AddBuiltinScriptFiles(list);
    std::string address = options.GetOption("address");
    std::string port = options.GetOption("port");
    RPCServer srv(pref, &loader);
    if (address.size() == 0) {
        address = "localhost";
    }
    ServeTCP(&srv, port, address);
    delete pFileIO;
    delete pBuiltinFileIO;
}

void DoNVTTest(Interpreter::Value& pref, ParseArgs& option) {
    NVTPref helper(pref);
    if (helper.script_folder().size() == 0 && helper.script_package().size() == 0) {
        std::cout << "You must provider script_folder or script_package in you config file"
                  << std::endl;
        return;
    }
    std::string hostList = option.GetOption("hosts");
    std::string portList = option.GetOption("ports");

    if (hostList.size() == 0 || hostList.size() == 0) {
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

            HostsTask task(hostList, portList, pref, &loader);
            task.Start(oidList);
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

        HostsTask task(hostList, portList, pref, &loader);
        task.Start(oidList);
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

        HostsTask task(hostList, portList, pref, &loader);
        task.Start(oidList);
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

void init() {
    masscan_init();
    Interpreter::InitializeLibray();
    net::Socket::InitLibrary();
    ssh_init();
    SSL_load_error_strings();
    SSL_library_init();
}

//NVTEngine daemon -a localhost -p 100 -c
//NVTEngine scan -h -p -c -f
//NVTEngine update -c

int main(int argc, char* argv[]) {
    Command daemonCmd("daemon", "run jsonrpc server", std::list<Option>());
    daemonCmd.options.push_back(Option("c", "config", "config file path", false));
    daemonCmd.options.push_back(
            Option("a", "address", "listen address,default localhost", false));
    daemonCmd.options.push_back(Option("p", "port", "listen port", true));

    Command scanCmd("scan", "start cli to scan target", std::list<Option>());
    scanCmd.options.push_back(Option("c", "config", "config file path", false));
    scanCmd.options.push_back(Option("h", "hosts", "target hosts list", true));
    scanCmd.options.push_back(Option("p", "ports", "port range", true));
    scanCmd.options.push_back(Option("f", "filter", "filter to select scripts", false));

    Command updateCmd("update", "update NVTI database", std::list<Option>());
    updateCmd.options.push_back(Option("c", "config", "config file path", false));

    std::list<Command> allCommands;
    allCommands.push_back(daemonCmd);
    allCommands.push_back(scanCmd);
    allCommands.push_back(updateCmd);

#ifdef __APPLE__
    signal(SIGPIPE, SIG_IGN);
#endif
    init();

    ParseArgs options(argc, argv, allCommands);
    if (!options.IsValid()) {
        options.PrintHelp("NVTEngine");
        return -1;
    }
    std::string configFile = options.GetOption("config");
    Interpreter::Value pref;
    if (configFile.size() == 0) {
        if (!LoadDefaultConfig(pref)) {
            options.PrintHelp("NVTEngine");
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
    if (options.GetCommand() == daemonCmd.strCmd) {
        ServeDaemon(options, pref);
        return 0;
    }
    if (options.GetCommand() == scanCmd.strCmd) {
        DoNVTTest(pref, options);
        pref = Value();
        std::cout << Interpreter::Status::ToString() << std::endl;
        return 0;
    }
    if (options.GetCommand() == updateCmd.strCmd) {
        UpdateNVTI(pref);
        return 0;
    }
    options.PrintHelp("NVTEngine");
    return -1;
}