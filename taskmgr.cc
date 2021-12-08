extern "C" {
#include "thirdpart/masscan/hostscan.h"
#include "thirdpart/masscan/pixie-threads.h"
#include "thirdpart/masscan/pixie-timer.h"
}
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/api.hpp"
#include "modules/openvas/support/nvtidb.hpp"
#include "taskmgr.hpp"

HostsTask::HostsTask(std::string host, std::string ports, Value& prefs, FileIO* IO)
        : mScriptCount(0), mHosts(host), mPorts(ports), mPrefs(prefs), mMainThread(0), mIO(IO) {
    masscan_init();
}

bool HostsTask::BeginTask(std::list<std::string>& scripts, std::string TaskID) {
    if (IsRuning()) {
        return true;
    }
    if (!InitScripts(scripts)) {
        return false;
    }
    mMainThread = pixie_begin_thread(HostsTask::ExecuteThreadProxy, 0, this);
    mTaskID = TaskID;
    return true;
}

void HostsTask::Join() {
    pixie_thread_join(mMainThread);
}

void HostsTask::Execute() {
    MassIP target;
    massip_add_target_string(&target, mHosts.c_str());
    massip_add_port_string(&target, mPorts.c_str(), 0);
    bool bIsNeedARP = false;
    struct ARPItem* arpItem = NULL;
    unsigned int arpItemSize = 0;

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
    HostScanTask* task = init_host_scan_task(mHosts.c_str(), mPorts.c_str(), false, false, "",
                                             arpItem, arpItemSize);
    if (task == NULL) {
        LOG("init_host_scan_task failed");
        return;
    }
    start_host_scan_task(task);
    while (!task->send_done) {
        pixie_mssleep(1000);
    }
    pixie_mssleep(1000 * 10);
    join_host_scan_task(task);
    HostScanResult* seek = task->result;
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
        std::string tcps = ",", udps = ",";
        for (unsigned int i = 0; i < seek->size; i++) {
            if (TCP_UNKNOWN == seek->list[i].type) {
                std::string name = "Ports/tcp/" + ToString(seek->list[i].port);
                tcb->Storage->SetItem(name, Value(1));
                tcps += ToString(seek->list[i].port);
                tcps += ",";
                continue;
            }
            if (UDP_UNKNOWN == seek->list[i].type) {
                std::string name = "Ports/udp/" + ToString(seek->list[i].port);
                tcb->Storage->SetItem(name, Value(1));
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
            for (auto v : mPrefs._map()) {
                tcb->Env[v.first] = v.second;
            }
            tcb->Storage->SetItem("testname", "name1");
            tcb->Storage->SetItem("testname", "name2");
            tcb->Storage->SetItem("testname1", "name3");
            tcb->Storage->SetItem("testname1", "name4");
            tcb->Storage->SetItem("testname1", "name5");
            //tcb->Storage->AddService("www", 80);
            tcb->Storage->AddService("www", 443);
        }
        tcb->Task = this;
        tcb->ThreadHandle = pixie_begin_thread(HostsTask::ExecuteOneHostThreadProxy, 0, tcb);
        mTCBGroup.push_back(tcb);
        //TODO check we can start more task??
        seek = seek->next;
    }
    //wait_all_thread_exit
    destory_host_scan_task(task);
    if (arpItem != NULL) {
        free(arpItem);
    }
    for (auto iter : mTCBGroup) {
        pixie_thread_join(iter->ThreadHandle);
        //TODO copy result
        delete (iter);
    }
}

void HostsTask::ExecuteOneHost(TCB* tcb) {
    //TODO find service
    ExecuteScriptOnHost(tcb);
}

void HostsTask::ExecuteScriptOnHost(TCB* tcb) {
    DefaultExecutorCallback callback(mPrefs["scripts_folder"].bytes, mIO);
    Interpreter::Executor Engine(&callback, NULL);
    RegisgerModulesBuiltinMethod(&Engine);
    LOG("\n");
    for (int i = 0; i < 11; i++) {
        for (auto v : mGroupedScripts[i]) {
            OVAContext ctx(v[knowntext::kNVTI_filename].bytes, mPrefs, tcb->Env, tcb->Storage);
            ctx.Nvti = v;
            ctx.Host = tcb->Host;
            if (!CheckScript(&ctx, v)) {
                continue;
            }
            LOG("execute script " + ctx.ScriptFileName);
            Engine.SetUserContext(&ctx);
            Engine.Execute(ctx.ScriptFileName.c_str(), false);
            if (tcb->Exit) {
                break;
            }
            if (ctx.Fork.IsForked()) {
                ctx.Fork.Snapshot = ctx.Storage->Clone();
                ctx.IsForkedTask = true;
                while (ctx.Fork.PrepareForNextScriptExecute() && !tcb->Exit) {
                    LOG("execute forked script " + ctx.ScriptFileName);
                    for (auto v : ctx.Fork.Names) {
                        LOG("forked value named:", v);
                    }
                    Engine.Execute(ctx.ScriptFileName.c_str(), false);
                }
            }
            tcb->ScriptCount++;
        }
        if (tcb->Exit) {
            break;
        }
    }
}

bool HostsTask::InitScripts(std::list<std::string>& scripts) {
    support::NVTIDataBase nvtiDB("attributes.db");
    support::Prefs prefsDB("prefs.db");
    std::map<std::string, int> loaded;
    std::list<Value> loadOrder;
    bool loadDep = true;
    if (mPrefs[knowntext::kPref_load_dependencies] == "no") {
        loadDep = false;
    }
    for (int i = 0; i < 11; i++) {
        mGroupedScripts[i] = std::list<Value>();
    }
    for (auto v : scripts) {
        Value nvti = nvtiDB.Get(v);
        if (nvti.IsNULL()) {
            LOG("load " + v + " failed");
            //return false;
            continue;
        }
        Value pref = prefsDB.Get(v);
        if (pref.IsObject()) {
            nvti[knowntext::kNVTI_preference] = pref;
        }
        loaded[nvti[knowntext::kNVTI_filename].bytes] = loaded.size();
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
            deps.push_back(x.bytes);
        }
        if (!InitScripts(nvtiDB, prefsDB, deps, loadOrder, loaded)) {
            return false;
        }
        loadOrder.push_back(nvti);
    }

    mScriptCount = loadOrder.size();
    for (auto nvti : loadOrder) {
        mGroupedScripts[nvti[knowntext::kNVTI_category].Integer].push_back(nvti);
    }
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
            LOG("load " + v + " failed");
            return false;
        }
        Value pref = prefsDB.Get(nvti[knowntext::kNVTI_oid].bytes);
        if (pref.IsObject()) {
            nvti[knowntext::kNVTI_preference] = pref;
        }
        loaded[v] = loaded.size();
        std::list<std::string> deps;
        Value dp = nvti[knowntext::kNVTI_dependencies];
        if (dp.IsNULL()) {
            loadOrder.push_back(nvti);
            continue;
        }
        for (auto x : dp._array()) {
            deps.push_back(x.bytes);
        }
        if (!InitScripts(nvtiDB, prefsDB, deps, loadOrder, loaded)) {
            return false;
        }
        loadOrder.push_back(nvti);
    }
    return true;
}

bool HostsTask::CheckScript(OVAContext* ctx, Value& nvti) {
    Value mandatory_keys = nvti[knowntext::kNVTI_mandatory_keys];
    if (mandatory_keys.IsObject()) {
        for (auto v : mandatory_keys._array()) {
            Value val = ctx->Storage->GetItem(v.bytes, true);
            if (val.IsNULL()) {
                std::cout << "skip script " << nvti[knowntext::kNVTI_filename].ToString()
                          << " because mandatory key is missing :" << v.ToString() << std::endl;
                return false;
            }
        }
    }

    Value require_keys = nvti[knowntext::kNVTI_require_keys];
    if (require_keys.IsObject()) {
        for (auto v : require_keys._array()) {
            Value val = ctx->Storage->GetItem(v.bytes, true);
            if (val.IsNULL()) {
                std::cout << "skip script " << nvti[knowntext::kNVTI_filename].ToString()
                          << " because require key is missing :" << v.ToString() << std::endl;
                return false;
            }
        }
    }

    Value require_ports = nvti[knowntext::kNVTI_require_ports];
    if (require_ports.IsObject()) {
        for (auto v : require_ports._array()) {
            if (!ctx->IsPortInOpenedRange(v, true)) {
                std::cout << "skip script " << nvti[knowntext::kNVTI_filename].ToString()
                          << " because require_ports is missing :" << v.ToString() << std::endl;
                return false;
            }
        }
    }

    Value require_udp_ports = nvti[knowntext::kNVTI_require_udp_ports];
    if (require_udp_ports.IsObject()) {
        for (auto v : require_udp_ports._array()) {
            if (!ctx->IsPortInOpenedRange(v, false)) {
                std::cout << "skip script " << nvti[knowntext::kNVTI_filename].ToString()
                          << " because require_udp_ports is missing :" << v.ToString() << std::endl;
                return false;
            }
        }
    }

    Value exclude_keys = nvti[knowntext::kNVTI_exclude_keys];
    if (exclude_keys.IsObject()) {
        for (auto v : exclude_keys._array()) {
            Value val = ctx->Storage->GetItem(v.bytes, true);
            if (!val.IsNULL()) {
                std::cout << "skip script " << nvti[knowntext::kNVTI_filename].ToString()
                          << " because exclude_keys is exist :" << v.ToString() << std::endl;
                return false;
            }
        }
    }
    return true;
}