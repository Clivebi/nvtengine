#pragma once
#include <string>
namespace knowntext {
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

const std::string kENV_opened_tcp = "opened_tcp";
const std::string kENV_opened_udp = "opened_udp";
const std::string kENV_local_ip = "local_ip";
const std::string kENV_default_ifname = "default_ifname";
const std::string kENV_route_ip = "route_ip";
const std::string kENV_route_mac = "route_mac";
const std::string kENV_local_mac = "local_mac";
} // namespace knowntext