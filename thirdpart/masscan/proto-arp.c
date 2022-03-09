#include "proto-arp.h"
#include "proto-preprocess.h"
#include "logger.h"
#include "output.h"
#include "masscan-status.h"
#include "unusedparm.h"
#include "hostscan.h"
#include "stack-arpv4.h"
#include "string_s.h"


void
handle_arp_response(struct HostScanTask* task, time_t timestamp, const unsigned char *px,
                         unsigned length, struct PreprocessedInfo *parsed)
{
    struct ARP_IncomingRequest req = {0};
    ipaddress ip_them = parsed->src_ip;
    ipaddress_formatted_t fmt = ipaddress_fmt(ip_them);
    struct HostScanResult * result = lookup_or_new_host_scan_result(task, ip_them);
    proto_arp_parse(&req, px, parsed->found_offset, length-parsed->found_offset);
    if (!req.is_valid) {
        return;
    }
    memcpy(result->mac_address.addr, req.mac_src,6);
    
    UNUSEDPARM(length);
    
    LOG(3, "ARP %s = [%02X:%02X:%02X:%02X:%02X:%02X]\n",
        fmt.string,
        result->mac_address.addr[0], result->mac_address.addr[1], result->mac_address.addr[2],
        result->mac_address.addr[3], result->mac_address.addr[4], result->mac_address.addr[5]);
    
}
