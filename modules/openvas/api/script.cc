#include "../api.hpp"

Value script_name(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["name"] = args.front();
    return Value();
}

Value script_version(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["version"] = args.front();
    return Value();
}

Value script_timeout(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["timeout"] = args.front();
    return Value();
}

Value script_copyright(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return Value();
}

Value script_family(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["family"] = args.front();
    return Value();
}

Value script_require_keys(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_STRINGS();
    script->Nvti["require_keys"] = Value(args);
    return Value();
}

Value script_mandatory_keys(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_STRINGS();
    script->Nvti["mandatory_keys"] = Value(args);
    return Value();
}

Value script_require_ports(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["require_ports"] = Value(args);
    return Value();
}

Value script_require_udp_ports(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["require_udp_ports"] = Value(args);
    return Value();
}

Value script_exclude_keys(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_STRINGS();
    script->Nvti["exclude_keys"] = Value(args);
    return Value();
}
//#script_add_preference(name: "Prefix directory", type: "entry", value: "/etc/apache2/", id: 1)
Value script_add_preference(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    CHECK_PARAMETER_INTEGER(3);
    script->AddPreference(args[3], args[0], args[1], args[2]);
    return Value();
}

//func script_get_preference(name,id)
Value script_get_preference(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_INTEGER(1);
    return script->GetPreference(args[1], args[0]);
}

//func script_get_preference_file_content(name,id)
Value script_get_preference_file_content(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_INTEGER(1);
    Value ret = "";
    script->GetPreferenceFile(args[1], args[0], ret.bytes);
    return ret;
}

Value script_get_preference_file_location(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return "";
}

Value script_oid(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["oid"] = args.front();
    return Value();
}

Value script_cve_id(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_STRINGS();
    script->Nvti["cve_id"] = Value(args);
    return Value();
}

Value script_bugtraq_id(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["bugtraq_id"] = Value(args);
    return Value();
}

Value script_xref(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    OVAContext* script = GetOVAContext(vm);
    script->AddXref(args[0], args[1]);
    return Value();
}

Value script_tag(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    OVAContext* script = GetOVAContext(vm);
    script->AddTag(args[0], args[1]);
    return Value();
}

Value script_category(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["category"] = args[0];
    return Value();
}

Value script_dependencies(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_STRINGS();
    OVAContext* script = GetOVAContext(vm);
    script->Nvti["dependencies"] = args;
    return Value();
}

Value set_kb_item(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Storage->SetItem(args[0].bytes, args[1]);
    return Value();
}

Value get_kb_item(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    return script->Storage->GetItem(args[0].bytes, true);
}

Value get_kb_list(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    return script->Storage->GetItemList(args[0].bytes);
}

Value get_preference(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    return script->Prefs[args.front()];
}

Value get_script_oid(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    return script->Nvti["oid"];
}

Value replace_kb_item(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    OVAContext* script = GetOVAContext(vm);
    script->Storage->ReplaceItem(args[0].bytes, args[1]);
    return Value();
}

Value security_message(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    auto iter = args.begin();
    std::cout << "MSG\t";
    while (iter != args.end()) {
        std::cout << iter->ToString() << ",";
        iter++;
    }
    std::cout << std::endl;
    return Value();
}

Value log_message(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    auto iter = args.begin();
    std::cout << "LOG \t";
    while (iter != args.end()) {
        std::cout << iter->ToString() << ",";
        iter++;
    }
    std::cout << std::endl;
    return Value();
}

Value error_message(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    auto iter = args.begin();
    std::cout << "ERROR\t";
    while (iter != args.end()) {
        std::cout << iter->ToString() << ",";
        iter++;
    }
    std::cout << std::endl;
    return Value();
}

Value get_host_ip(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    return script->Host;
}

Value HostEnv(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    return script->Env;
}