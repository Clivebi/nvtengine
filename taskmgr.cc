extern "C" {
#include "thirdpart/masscan/hostscan.h"
#include "thirdpart/masscan/pixie-threads.h"
#include "thirdpart/masscan/pixie-timer.h"
}
#include <regex>

#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/api.hpp"
#include "modules/openvas/support/creddb.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "ntvpref.hpp"
#include "taskmgr.hpp"

HostsTask::HostsTask(std::string host, std::string ports, Value& prefs, ScriptLoader* IO)
        : mScriptCount(0),
          mHosts(host),
          mPorts(ports),
          mPrefs(prefs),
          mMainThread(0),
          mLoader(IO),
          mScriptCache(),
          mTaskCount() {}

bool HostsTask::BeginTask(std::list<std::string>& scripts, std::string TaskID) {
    if (IsRuning()) {
        return true;
    }
    if (!InitScripts(scripts)) {
        return false;
    }
    mTaskCount = AtomInt();
    mMainThread = pixie_begin_thread(HostsTask::ExecuteThreadProxy, 0, this);
    mTaskID = TaskID;
    return true;
}

void HostsTask::Join() {
    pixie_thread_join(mMainThread);
}

void HostsTask::Execute() {
    NVTPref helper(mPrefs);
    MassIP target = {0};
    massip_add_target_string(&target, mHosts.c_str());
    massip_add_port_string(&target, mPorts.c_str(), 0);
    bool bIsNeedARP = false;
    struct ARPItem* arpItem = NULL;
    unsigned int arpItemSize = 0;
    time_t scannerTime = time(NULL);

    for (unsigned int i = 0; i < target.ipv4.count; i++) {
        ipaddress address;
        address.version = 4;
        address.ipv4 = target.ipv4.list[i].begin;
        if (is_private_address(address)) {
            bIsNeedARP = true;
        }
    }
    massip_destory(&target);

    if (bIsNeedARP) {
        arpItem = resolve_mac_address(mHosts.c_str(), &arpItemSize, 15);
    }
    HostScanTask* task = init_host_scan_task(mHosts.c_str(), mPorts.c_str(), false, false,
                                             helper.default_network_interface().c_str(), arpItem,
                                             arpItemSize, helper.port_scan_send_packet_rate());
    if (task == NULL) {
        NVT_LOG_ERROR("init_host_scan_task failed");
        return;
    }
    start_host_scan_task(task);
    while (!task->send_done) {
        pixie_mssleep(1000);
    }
    pixie_mssleep(1000 * 15);
    join_host_scan_task(task);
    NVT_LOG_DEBUG("ports scan complete...");
    scannerTime = time(NULL) - scannerTime;
    HostScanResult* seek = task->result;
    unsigned int taskLimit = helper.number_of_concurrent_tasks();
    unsigned int totalTaskCount = 0, scheduledTaskCount = 0;
    while (seek != NULL) {
        totalTaskCount++;
        seek = seek->next;
    }
    seek = task->result;
    while (seek != NULL) {
        ipaddress_formatted_t host = ipaddress_fmt(seek->address);
        TCB* tcb = new TCB(host.string);
        tcb->Exit = false;
        for (unsigned int i = 0; i < arpItemSize; i++) {
            if (ipaddress_is_equal(seek->address, arpItem[i].ip)) {
                ipaddress_formatted_t mac = macaddress_fmt(arpItem[i].mac);
                tcb->Storage->SetItem("Host/mac_address", mac.string);
                break;
            }
        }
        std::vector<int> portsTCP;
        std::vector<int> portsUDP;
        std::string tcps = ",", udps = ",";
        for (unsigned int i = 0; i < seek->size; i++) {
            if (TCP_UNKNOWN == seek->list[i].type) {
                std::string name = "Ports/tcp/" + ToString(seek->list[i].port);
                tcb->Storage->SetItem(name, Value(1));
                tcps += ToString(seek->list[i].port);
                tcps += ",";
                portsTCP.push_back(seek->list[i].port);
                continue;
            }
            if (UDP_UNKNOWN == seek->list[i].type) {
                std::string name = "Ports/udp/" + ToString(seek->list[i].port);
                tcb->Storage->SetItem(name, Value(1));
                portsUDP.push_back(seek->list[i].port);
            }
            if (UDP_CLOSED != seek->list[i].type) {
                udps += ToString(seek->list[i].port);
                udps += ",";
                continue;
            }
        }
        {
            ipaddress addr = {0};
            addr.ipv4 = task->src_ipv4;
            addr.version = 4;
            ipaddress_formatted_t fmtV = ipaddress_fmt(addr);

            tcb->Env[knowntext::kENV_local_ip] = fmtV.string;
            tcb->Env[knowntext::kENV_default_ifname] = task->nic.ifname;

            addr.ipv4 = task->nic.router_ip;
            fmtV = ipaddress_fmt(addr);
            tcb->Env[knowntext::kENV_route_ip] = fmtV.string;

            fmtV = macaddress_fmt(task->nic.source_mac);
            tcb->Env[knowntext::kENV_local_mac] = fmtV.string;

            fmtV = macaddress_fmt(task->nic.router_mac_ipv4);
            tcb->Env[knowntext::kENV_route_mac] = fmtV.string;
            tcb->Env[knowntext::kENV_opened_tcp] = tcps;
            tcb->Env[knowntext::kENV_opened_udp] = udps;
            tcb->ScannerTime = scannerTime;
            //tcb->Storage->SetItem("Settings/disable_cgi_scanning",true);
            tcb->Storage->SetItem("default_credentials/disable_brute_force_checks", true);
        }
        tcb->TCPPorts = portsTCP;
        tcb->UDPPorts = portsUDP;
        tcb->Task = this;
        scheduledTaskCount++;
        std::cout << "begin task thread for the target: " << tcb->Host
                  << " scheduled task: " << scheduledTaskCount
                  << " total task count: " << totalTaskCount
                  << " running task count: " << mTaskCount.Get() << std::endl;
        tcb->ThreadHandle = pixie_begin_thread(HostsTask::ExecuteOneHostThreadProxy, 0, tcb);
        mTCBGroup.push_back(tcb);
        while (mTaskCount.Get() >= taskLimit) {
            pixie_mssleep(200);
        }
        seek = seek->next;
    }
    destory_host_scan_task(task);
    if (arpItem != NULL) {
        free(arpItem);
    }
    for (auto iter : mTCBGroup) {
        pixie_thread_join(iter->ThreadHandle);
        delete (iter);
    }
}

void HostsTask::LoadCredential(TCB* tcb) {
    NVTPref helper(mPrefs);
    support::CredDB db(FilePath(helper.app_data_folder()) + "cred.db");
    Value res = db.Get(tcb->Host);
    if (res.IsMap()) {
        Value ssh = res["ssh"];
        Value winrm = res["winrm"];
        if (ssh.IsMap()) {
            tcb->Storage->SetItem("Secret/SSH/login", ssh["login"]);
            tcb->Storage->SetItem("Secret/SSH/password", ssh["password"]);
            tcb->Storage->SetItem("Secret/SSH/privatekey", ssh["privatekey"]);
            tcb->Storage->SetItem("Secret/SSH/passphrase", ssh["passphrase"]);
            if (ssh["port"].IsInteger()) {
                tcb->Storage->SetItem("Secret/SSH/transport", ssh["port"]);
            }
        }
        if (winrm.IsMap()) {
            tcb->Storage->SetItem("Secret/WinRM/login", winrm["login"]);
            tcb->Storage->SetItem("Secret/WinRM/password", winrm["password"]);
            if (winrm["port"].IsInteger()) {
                tcb->Storage->SetItem("Secret/WinRM/transport", winrm["port"]);
            }
            tcb->Storage->SetItem("WinRM/Connect/Exist", 1);
        }
    }
}

void HostsTask::ExecuteOneHost(TCB* tcb) {
    NVTPref helper(mPrefs);
    LoadCredential(tcb);
    NVT_LOG_DEBUG("\n", tcb->Host, " start detect service");
    TCPDetectService(tcb, tcb->TCPPorts, helper.service_detection_thread_count());
    NVT_LOG_DEBUG(tcb->Host, " detect service complete...");
    if (tcb->Exit) {
        return;
    }
    ExecuteScriptOnHost(tcb);
    OutputHostResult(tcb);
}

void HostsTask::OutputTextResult(TCB* tcb) {
    std::stringstream o;
    o << std::endl << "----begin output result----" << std::endl;
    o << "target: " << tcb->Host << std::endl;
    o << "count_of_script: " << tcb->ExecutedScriptCount << std::endl;
    o << "script_start_time: " << (long)tcb->BirthTime << std::endl;
    o << "script_time_second: " << (long)(time(NULL) - tcb->BirthTime) << std::endl;
    o << "scanner_time_second: " << (long)tcb->ScannerTime << std::endl;
    o << "script_env: " << tcb->Env.ToString() << std::endl;

    Value logs = tcb->Storage->GetItemList("script/logs");
    if (logs.IsArray()) {
        for (auto item : logs._array()) {
            if (item.IsArray()) {
                for (auto v : item._array()) {
                    o << v.ToString();
                    o << " , ";
                }
                o << std::endl;
            } else {
                o << item.ToString() << std::endl;
            }
        }
    }

    Value result = tcb->Storage->GetItemKeys("HostDetails*");
    if (result.IsArray()) {
        for (auto v : result._array()) {
            Value dataList = tcb->Storage->GetItemList(v.ToString());
            o << v.ToString() << ": " << dataList.ToString() << std::endl;
        }
    }
    o << std::endl << "----end output result----" << std::endl;
    tcb->Report = o.str();
    std::cout << tcb->Report << std::endl;
}

void HostsTask::OutputJsonResult(TCB* tcb) {
    std::stringstream o;
    Value total = Value::MakeMap();
    Value result = tcb->Storage->GetItemList("script/logs");
    total["logs"] = result;
    total["target"] = tcb->Host;
    total["count_of_script"] = tcb->ExecutedScriptCount;
    total["script_start_time"] = (long)tcb->BirthTime;
    total["script_time_second"] = (long)(time(NULL) - tcb->BirthTime);
    total["scanner_time_second"] = (long)tcb->ScannerTime;
    total["script_env"] = tcb->Env;
    Value detail = Value::MakeArray();
    total["details"] = detail;

    result = tcb->Storage->GetItemKeys("HostDetails*");
    if (result.IsArray()) {
        for (auto v : result._array()) {
            Value dataList = tcb->Storage->GetItemList(v.ToString());
            Value item = Value::MakeMap();
            item["key"] = v;
            item["value"] = dataList;
            detail._array().push_back(item);
        }
    }
    tcb->Report = total.ToJSONString();
    std::cout << tcb->Report << std::endl;
}

void HostsTask::OutputHostResult(TCB* tcb) {
    NVTPref helper(mPrefs);
    if (helper.result_output_format() == "json") {
        OutputJsonResult(tcb);
    } else {
        OutputTextResult(tcb);
    }
    std::cout << Status::ToString();
    //when have too much host,the storage used a large memory,
    //so after generic report,free it
    tcb->Storage = NULL;
    std::cout << Status::ToString();
}

void HostsTask::ExecuteScriptOnHost(TCB* tcb) {
    NVTPref helper(mPrefs);
    DefaultExecutorCallback callback;
    Interpreter::Executor Engine(&callback, mLoader);
    Engine.SetScriptCacheProvider(&mScriptCache);
    RegisgerModulesBuiltinMethod(&Engine);
    NVT_LOG_DEBUG("start execute script");
    for (int i = 0; i < 11; i++) {
        for (auto v : mGroupedScripts[i]) {
            OVAContext ctx(v[knowntext::kNVTI_filename].ToString(), mPrefs, tcb->Env, tcb->Storage);
            ctx.Nvti = v;
            ctx.Host = tcb->Host;
            tcb->ScriptProgress++;
            if (!CheckScript(&ctx, v)) {
                continue;
            }
            NVT_LOG_DEBUG("execute script " + ctx.ScriptFileName);
            Engine.SetUserContext(&ctx);
            Engine.Execute(ctx.ScriptFileName.c_str(), helper.script_default_timeout_second(),
                           helper.log_engine_warning());
            if (tcb->Exit) {
                break;
            }
            if (ctx.Fork.IsForked()) {
                ctx.Fork.Snapshot = ctx.Storage->Clone();
                ctx.IsForkedTask = true;
                while (ctx.Fork.PrepareForNextScriptExecute() && !tcb->Exit) {
                    // NVT_LOG_DEBUG("execute forked script " + ctx.ScriptFileName);
                    for (auto v : ctx.Fork.Names) {
                        //     NVT_LOG_DEBUG("forked value named:", v);
                    }
                    Engine.Execute(ctx.ScriptFileName.c_str(),
                                   helper.script_default_timeout_second(),
                                   helper.log_engine_warning());
                }
            }
            tcb->ExecutedScriptCount++;
        }
        if (tcb->Exit) {
            break;
        }
    }
    NVT_LOG_DEBUG("All script complete.... Total Count: ", mScriptCount,
                  " Executed count: ", tcb->ExecutedScriptCount);
}

void HostsTask::ThinNVTI(Value& nvti, bool lastPhase) {
    /*
    const std::string kNVTI_oid = "oid";
const std::string kNVTI_name = "name";
const std::string kNVTI_version = "version";
const std::string kNVTI_timeout = "timeout";
const std::string kNVTI_family = "family";
const std::string kNVTI_require_keys = "require_keys";
const std::string kNVTI_mandatory_keys = "mandatory_keys";
const std::string kNVTI_require_ports = "require_ports";
const std::string kNVTI_require_udp_ports = "require_udp_ports";
const std::string kNVTI_exclude_keys = "exclude_keys";
const std::string kNVTI_cve_id = "cve_id";
const std::string kNVTI_bugtraq_id = "bugtraq_id";
const std::string kNVTI_category = "category";
const std::string kNVTI_dependencies = "dependencies";
const std::string kNVTI_filename = "filename";
const std::string kNVTI_preference = "preference";
const std::string kNVTI_xref = "xref";
const std::string kNVTI_tag = "tag";
    */
    if (nvti.IsMap()) {
        nvti._map().erase(knowntext::kNVTI_name);
        nvti._map().erase(knowntext::kNVTI_version);
        nvti._map().erase(knowntext::kNVTI_cve_id);
        nvti._map().erase(knowntext::kNVTI_bugtraq_id);
        nvti._map().erase(knowntext::kNVTI_xref);
        nvti._map().erase(knowntext::kNVTI_tag);
        if (lastPhase) {
            nvti._map().erase(knowntext::kNVTI_dependencies);
            nvti._map().erase(knowntext::kNVTI_family);
            nvti._map().erase(knowntext::kNVTI_category);
        }
    }
}
bool HostsTask::InitScripts(std::list<std::string>& scripts) {
    NVTPref helper(mPrefs);
    support::NVTIDataBase nvtiDB(FilePath(helper.app_data_folder()) + "attributes.db");
    support::Prefs prefsDB(FilePath(helper.app_data_folder()) + "prefs.db");
    std::map<std::string, int> loaded;
    std::list<Value> loadOrder;
    bool loadDep = true;
    for (int i = 0; i < 11; i++) {
        mGroupedScripts[i] = std::list<Value>();
    }
    for (auto v : scripts) {
        Value nvti = nvtiDB.Get(v);
        if (nvti.IsNULL()) {
            NVT_LOG_ERROR("load " + v + " failed");
            //return false;
            continue;
        }
        ThinNVTI(nvti, false);
        if (loaded.find(nvti[knowntext::kNVTI_filename].ToString()) != loaded.end()) {
            continue;
        }
        Value pref = prefsDB.Get(v);
        if (pref.IsMap()) {
            nvti[knowntext::kNVTI_preference] = pref;
        }
        loaded[nvti[knowntext::kNVTI_filename].ToString()] = (int)loaded.size();
        if (!loadDep) {
            loadOrder.push_back(nvti);
            continue;
        }
        std::list<std::string> deps;
        Value dp = nvti[knowntext::kNVTI_dependencies];
        if (dp.IsNULL()) {
            loadOrder.push_back(nvti);
            continue;
        }
        for (auto x : dp._array()) {
            deps.push_back(x.ToString());
        }
        if (!InitScripts(nvtiDB, prefsDB, deps, loadOrder, loaded)) {
            return false;
        }
        loadOrder.push_back(nvti);
    }

    mScriptCount = loadOrder.size();
    for (auto nvti : loadOrder) {
        mGroupedScripts[nvti[knowntext::kNVTI_category].Integer].push_back(nvti);
        ThinNVTI(nvti, true);
    }
    NVT_LOG_DEBUG("Total load NVTI count:", mScriptCount);
    return true;
}

bool HostsTask::InitScripts(support::NVTIDataBase& nvtiDB, support::Prefs& prefsDB,
                            std::list<std::string>& scripts, std::list<Value>& loadOrder,
                            std::map<std::string, int>& loaded) {
    for (auto v : scripts) {
        if (loaded.find(v) != loaded.end()) {
            continue;
        }
        Value nvti = nvtiDB.GetFromFileName(v);
        if (nvti.IsNULL()) {
            NVT_LOG_ERROR("load " + v + " failed");
            return false;
        }
        ThinNVTI(nvti, false);
        Value pref = prefsDB.Get(nvti[knowntext::kNVTI_oid].ToString());
        if (pref.IsMap()) {
            nvti[knowntext::kNVTI_preference] = pref;
        }
        loaded[v] = (int)loaded.size();
        std::list<std::string> deps;
        Value dp = nvti[knowntext::kNVTI_dependencies];
        if (dp.IsNULL()) {
            loadOrder.push_back(nvti);
            continue;
        }
        for (auto x : dp._array()) {
            deps.push_back(x.ToString());
        }
        if (!InitScripts(nvtiDB, prefsDB, deps, loadOrder, loaded)) {
            return false;
        }
        loadOrder.push_back(nvti);
    }
    return true;
}

bool HostsTask::CheckScript(OVAContext* ctx,const Value& nvti) {
    Value mandatory_keys =  nvti[knowntext::kNVTI_mandatory_keys];
    if (mandatory_keys.IsArray()) {
        for (auto v : mandatory_keys._array()) {
            if (v.ToString().find("=") != std::string::npos) {
                std::list<std::string> group = split(v.ToString(), '=');
                Value val = ctx->Storage->GetItem(group.front(), -1);
                if (val.IsNULL()) {
                    NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                                  " because mandatory key is missing :", v.ToString());
                    return false;
                }
                std::regex re = std::regex(group.back(), std::regex_constants::icase);
                bool found = std::regex_search(val.text.begin(), val.text.end(), re);
                if (!found) {
                    NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                                  " because mandatory key is missing :", v.ToString());
                    return false;
                }

            } else {
                Value val = ctx->Storage->GetItem(v.text, true);
                if (val.IsNULL()) {
                    NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                                  " because mandatory key is missing :", v.ToString());
                    return false;
                }
            }
        }
    }

    Value require_keys = nvti[knowntext::kNVTI_require_keys];
    if (require_keys.IsArray()) {
        for (auto v : require_keys._array()) {
            Value val = ctx->Storage->GetItem(v.text, true);
            if (val.IsNULL()) {
                NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                              " because require key is missing :", v.ToString());
                return false;
            }
        }
    }

    Value require_ports = nvti[knowntext::kNVTI_require_ports];
    if (require_ports.IsArray()) {
        bool found = false;
        for (auto v : require_ports._array()) {
            if (ctx->IsPortInOpenedRange(v, true)) {
                found = true;
            }
            if (v.IsInteger() && v.Integer == 139 &&
                (ctx->Storage->GetItem("WinRM/Connect/Exist", -1) == 1)) {
                found = true;
            }
        }

        if (!found) {
            NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                          " because not one require tcp port exist ", require_ports.ToString());
            return false;
        }
    }

    Value require_udp_ports = nvti[knowntext::kNVTI_require_udp_ports];
    if (require_udp_ports.IsArray()) {
        bool found = false;
        for (auto v : require_udp_ports._array()) {
            found = true;
        }
        if (!found) {
            NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                          " because not one require udp port exist ", require_udp_ports.ToString());
            return false;
        }
    }

    Value exclude_keys = nvti[knowntext::kNVTI_exclude_keys];
    if (exclude_keys.IsArray()) {
        for (auto v : exclude_keys._array()) {
            Value val = ctx->Storage->GetItem(v.text, true);
            if (!val.IsNULL()) {
                NVT_LOG_DEBUG("skip script " + nvti[knowntext::kNVTI_filename].ToString(),
                              " because exclude_keys is exist :", v.ToString());
                return false;
            }
        }
    }
    return true;
}

class DetectServiceCallback : public DefaultExecutorCallback {
protected:
    HostsTask::TCB* tcb;
    std::vector<int> ports;

public:
    DetectServiceCallback(HostsTask::TCB* tcb, std::vector<int>& ports)
            : DefaultExecutorCallback(), tcb(tcb), ports(ports) {}
    void OnScriptEntryExecuted(Executor* vm, scoped_refptr<const Script> Script, VMContext* ctx) {
        if (tcb->Exit) {
            return;
        }
        for (auto v : ports) {
            std::vector<Value> params;
            params.push_back(tcb->Host);
            params.push_back(v);
            vm->CallScriptFunction(std::string("detect_tcp_service"), params, ctx);
            if (tcb->Exit) {
                break;
            }
        }
    }
};

void HostsTask::DetectService(DetectServiceParamter* param) {
    NVTPref helper(mPrefs);
    DetectServiceCallback callback(param->tcb, param->ports);
    Interpreter::Executor Engine(&callback, mLoader);
    Engine.SetScriptCacheProvider(&mScriptCache);
    RegisgerModulesBuiltinMethod(&Engine);
    OVAContext ctx("servicedetect.sc", mPrefs, param->tcb->Env, param->storage);
    ctx.Host = param->tcb->Host;
    Engine.SetUserContext(&ctx);
    Engine.Execute(ctx.ScriptFileName.c_str(), helper.script_default_timeout_second(),
                   helper.log_engine_warning());
    if (param->tcb->Exit) {
        return;
    }
}

void HostsTask::TCPDetectService(TCB* tcb, const std::vector<int>& ports, size_t thread_count) {
    std::vector<thread_type> Threads;
    std::vector<DetectServiceParamter*> Paramters;
    std::vector<std::vector<int>> groups;
    if (ports.size() < thread_count) {
        thread_count = ports.size();
    }
    for (size_t i = 0; i < thread_count; i++) {
        groups.push_back(std::vector<int>());
    }
    for (size_t i = 0; i < ports.size(); i++) {
        groups[i % thread_count].push_back(ports[i]);
    }
    for (size_t i = 0; i < thread_count; i++) {
        DetectServiceParamter* param = new DetectServiceParamter();
        param->tcb = tcb;
        param->ports = groups[i];
        param->storage = tcb->Storage->Clone();
        Paramters.push_back(param);
        thread_type thread = pixie_begin_thread(HostsTask::DetectServiceProxy, 0, param);
        Threads.push_back(thread);
    }

    for (size_t i = 0; i < thread_count; i++) {
        pixie_thread_join(Threads[i]);
    }
    for (size_t i = 0; i < thread_count; i++) {
        tcb->Storage->Combine(Paramters[i]->storage);
        Paramters[i]->storage = NULL;
        delete (Paramters[i]);
    }
}