#include "proto-arp.h"
#include "proto-preprocess.h"
#include "logger.h"
#include "output.h"
#include "masscan-status.h"
#include "unusedparm.h"
#include "hostscan.h"


void
handle_arp_response(struct HostScanTask* task, time_t timestamp, const unsigned char *px,
                         unsigned length, struct PreprocessedInfo *parsed)
{
    ipaddress ip_them = parsed->src_ip;
    ipaddress_formatted_t fmt = ipaddress_fmt(ip_them);
    struct HostScanResult * result = lookup_or_new_host_scan_result(task, ip_them);
    result->mac_address.addr[0] = parsed->mac_src[0];
    result->mac_address.addr[1] = parsed->mac_src[1];
    result->mac_address.addr[2] = parsed->mac_src[2];
    result->mac_address.addr[3] = parsed->mac_src[3];
    result->mac_address.addr[4] = parsed->mac_src[4];
    result->mac_address.addr[5] = parsed->mac_src[5];
    
    UNUSEDPARM(length);
    UNUSEDPARM(px);
    
    LOG(3, "ARP %s = [%02X:%02X:%02X:%02X:%02X:%02X]\n",
        fmt.string,
        parsed->mac_src[0], parsed->mac_src[1], parsed->mac_src[2],
        parsed->mac_src[3], parsed->mac_src[4], parsed->mac_src[5]);
    
}
