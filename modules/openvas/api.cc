
#include "api.hpp"

#include "./api/crypto.cc"
#include "./api/net.cc"
#include "./api/packet.cc"
#include "./api/script.cc"
#include "./api/ssh.cc"
#include "./api/time.cc"
#include "./api/util.cc"
#include "./api/winrm.cc"
#include "ovacontext.cc"
#include "./api/snmp.cc"

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
        {"get_script_oid", get_script_oid},

        {"set_kb_item", set_kb_item},
        {"get_kb_item", get_kb_item},
        {"kb_get_keys", kb_get_keys},
        {"kb_get_list", kb_get_list},
        {"replace_kb_item", replace_kb_item},
        {"get_preference", get_preference},

        {"ResolveHostName", ResolveHostName},
        {"ResolveHostNameToList", ResolveHostNameToList},
        {"open_priv_sock_tcp", open_priv_sock_tcp},
        {"open_priv_sock_udp", open_priv_sock_udp},
        {"socket_get_error", socket_get_error},
        {"recv", recv},
        {"send", send},
        {"join_multicast_group", join_multicast_group},
        {"leave_multicast_group", leave_multicast_group},
        {"get_source_port", get_source_port},
        {"match", match},
        {"unixtime", UnixTime},
        {"gettimeofday", GetTimeOfDay},
        {"localtime", LocalTime},
        {"mktime", MakeTime},
        {"isotime_now", ISOTimeNow},
        {"isotime_print", ISOTimePrint},
        {"isotime_add", ISOTimeAdd},
        {"rand", Rand},
        {"usleep", USleep},
        {"sleep", Sleep},
        {"get_host_ip", get_host_ip},
        {"HostEnv", HostEnv},
        {"GetHostName", GetHostName},
        {"NASLString", NASLString},
        {"vendor_version", vendor_version},
        {"MD5", Md5Buffer},
        {"SHA1", SHA1Buffer},
        {"SHA256", SHA256Buffer},
        {"HMACMethod", HMACMethod},
        {"TLSPRF", TLSPRF},
        {"TLS1PRF", TLS1PRF},
        {"CipherOpen", CipherOpen},
        {"CipherUpdate", CipherUpdate},
        {"CipherFinal", CipherFinal},
        {"X509Open", X509Open},
        {"X509Query", X509Query},
        {"SSHConnect", SSHConnect},
        {"SSHAuth", SSHAuth},
        {"SSHLoginInteractive", SSHLoginInteractive},
        {"SSHExecute", SSHExecute},
        {"SSHGetIssueBanner", SSHGetIssueBanner},
        {"SSHGetServerBanner", SSHGetServerBanner},
        {"SSHGetPUBKey", SSHGetPUBKey},
        {"SSHGetAuthMethod", SSHGetAuthMethod},
        {"SSHShellOpen", SSHShellOpen},
        {"SSHShellRead", SSHShellRead},
        {"SSHShellWrite", SSHShellWrite},
        {"SSHIsSFTPEnabled", SSHIsSFTPEnabled},
        {"CreateWinRM", CreateWinRM},
        {"WinRMCommand", WinRMCommand},
        {"PcapSend", PcapSend},
        {"CapturePacket", CapturePacket},
        {"SNMPV1Get", SNMPV1Get},
        {"SNMPV2Get", SNMPV2Get},
        {"SNMPV3Get", SNMPV3Get},
};

void RegisgerOVABuiltinMethod(Executor* vm) {
    srand((unsigned int)time(NULL));
    vm->RegisgerFunction(ovaMethod, COUNT_OF(ovaMethod));
    vm->RegisgerFunction(ovaMethod, COUNT_OF(ovaMethod), "ova_");
}