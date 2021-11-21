
#include "api.hpp"

#include "./api/script.cc"
#include "base.cc"


BuiltinMethod ovaMethod[] = {
        {"script_name", script_name},
        {"script_version", script_version},
        {"script_timeout", script_timeout},
        {"script_copyright", script_copyright},
        {"script_category", script_category},
        {"script_family", script_family},
        {"script_dependencies", script_dependencies},
        {"script_require_keys", script_require_keys},
        {"script_mandatory_keys", script_mandatory_keys},
        {"script_require_ports", script_require_ports},
        {"script_require_udp_ports", script_require_udp_ports},
        {"script_exclude_keys", script_exclude_keys},
        {"script_add_preference", script_add_preference},
        {"script_get_preference", script_get_preference},
        {"script_get_preference_file_content", script_get_preference_file_content},
        {"script_get_preference_file_location", script_get_preference_file_location},
        {"script_oid", script_oid},
        {"script_cve_id", script_cve_id},
        {"script_bugtraq_id", script_bugtraq_id},
        {"script_xref", script_xref},
        {"script_tag", script_tag},
        {"set_kb_item", set_kb_item},
        {"get_kb_item", get_kb_item},
        {"get_kb_list", get_kb_list},
        {"replace_kb_item",replace_kb_item},
        {"get_preference",get_preference},
        {"get_script_oid",get_script_oid},
        {"security_message",security_message},
        {"log_message",log_message},
        {"error_message",error_message},
        {"get_script_oid",get_script_oid},
        };

void RegisgerOVABuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(ovaMethod, COUNT_OF(ovaMethod));
    vm->RegisgerFunction(ovaMethod, COUNT_OF(ovaMethod), "ova_");
}