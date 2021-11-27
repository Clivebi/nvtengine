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

HostsTask::HostsTask(std::string host, std::string ports, Value& prefs)
        : mScriptCount(0), mHosts(host), mPorts(ports), mPrefs(prefs), mMainThread(0) {
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
            ipaddress_formatted_t fmtIP = ipaddress_fmt(addr);
            tcb->Env["local_ip"] = fmtIP.string;
            tcb->Env["ifname"] = task->nic.ifname;
            addr.ipv4 = task->nic.router_ip;
            fmtIP = ipaddress_fmt(addr);
            tcb->Env["route_ip"] = fmtIP.string;
            tcb->Env["scanner_scanned"] = Value(true);
            tcb->Env["opened_tcp"] = tcps;
            tcb->Env["opened_udp"] = udps;
            for (auto v : mPrefs._map()) {
                tcb->Env[v.first] = v.second;
            }
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
        free(iter);
    }
}

void HostsTask::ExecuteOneHost(TCB* tcb) {
    //TODO find service
    ExecuteScriptOnHost(tcb);
}

void HostsTask::ExecuteScriptOnHost(TCB* tcb) {
    DefaultExecutorCallback callback(mPrefs["scripts_folder"].bytes);
    OVAContext ctx("");
    ctx.Host = tcb->Host;
    ctx.Storage = tcb->Storage;
    Interpreter::Executor Engine(&callback, &ctx);
    RegisgerModulesBuiltinMethod(&Engine);
    for (int i = 0; i < 11; i++) {
        Value root = mGroupedScripts[i];
        for (auto v : root._map()) {
            ctx.Name = v.second["filename"].bytes;
            ctx.Nvti = v.second;
            ctx.Env = tcb->Env;
            ctx.Prefs = mPrefs;
            //TODO check require keys
            Engine.Execute(ctx.Name.c_str(), false);
            if (tcb->Exit) {
                break;
            }
            tcb->ScriptCount++;
        }
        if (tcb->Exit) {
            break;
        }
    }
}

bool HostsTask::InitScripts(std::list<std::string>& scripts) {
    openvas::NVTIDataBase nvtiDB("attributes.db");
    std::map<std::string, int> loaded;
    for (int i = 0; i < 11; i++) {
        mGroupedScripts[i] = Value::make_map();
    }
    for (auto v : scripts) {
        Value nvti = nvtiDB.Get(v);
        if (nvti.IsNULL()) {
            LOG("load " + v + " failed");
            //return false;
            continue;
        }
        loaded[nvti["filename"].bytes] = 1;
        mGroupedScripts[nvti["category"].ToInteger()][v] = nvti;
        std::list<std::string> deps;
        Value dp = nvti["dependencies"];
        if (dp.IsNULL()) {
            continue;
        }
        for (auto x : dp._array()) {
            deps.push_back(x.bytes);
        }
        if (!InitScripts(nvtiDB, deps, loaded)) {
            return false;
        }
    }
    mScriptCount = 0;
    for (int i = 0; i < 11; i++) {
        mScriptCount += mGroupedScripts[i].Length();
    }
    return true;
}

bool HostsTask::InitScripts(openvas::NVTIDataBase& db, std::list<std::string>& scripts,
                            std::map<std::string, int>& loaded) {
    for (auto v : scripts) {
        if (loaded.find(v) != loaded.end()) {
            continue;
        }
        Value nvti = db.GetFromFileName(v);
        if (nvti.IsNULL()) {
            LOG("load " + v + " failed");
            return false;
        }
        loaded[v] = 1;
        mGroupedScripts[nvti["category"].ToInteger()][v] = nvti;
        std::list<std::string> deps;
        Value dp = nvti["dependencies"];
        if (dp.IsNULL()) {
            continue;
        }
        for (auto x : dp._array()) {
            deps.push_back(x.bytes);
        }
        if (!InitScripts(db, deps, loaded)) {
            return false;
        }
    }
    return true;
}