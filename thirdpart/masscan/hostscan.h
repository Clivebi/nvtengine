#ifndef hostscan_h
#define hostscan_h

#include "massip.h"
#include "stack-queue.h"
#include "stack-src.h"
#include "templ-pkt.h"
#define UDP_UNKNOWN (1000)
#define UDP_DNS (1001)
#define UDP_NETBIOS (1002)
#define UDP_NTP (1003)
#define UDP_SNMP (1004)
#define UDP_COAP (1005)
#define UDP_MEMCACHED (1006)
#define ICMP_REPLAY (1007)
#define UDP_CLOSED (1008)
#define STCP_OPEN (1009)
#define TCP_UNKNOWN (2001)

struct PortResult {
    unsigned type;
    unsigned port;
};

struct ARPItem {
    macaddress_t mac;
    ipaddress ip;
};

struct HostScanResult {
    unsigned res_alive : 1;
    unsigned res_mac : 1;
    unsigned res_tcp : 1;
    unsigned res_udp : 1;
    ipaddress address;
    macaddress_t mac_address;

    size_t size;
    struct PortResult* list;
    struct HostScanResult* next;
};

struct HostScanTask {
    unsigned icmp : 1;
    unsigned arp : 1;
    unsigned send_done : 1;
    unsigned recv_done : 1;
    unsigned shutdown;
    struct MassIP targets;
    uint64_t count_ipv4;
    uint64_t count_ipv6;
    uint64_t range;
    uint64_t range_ipv6;
    uint64_t rate;
    uint64_t retries;
    uint64_t send_count;
    uint64_t total_count;
    struct {
        uint64_t arp;
        uint64_t icmp;
    } count;
    struct {
        char ifname[256];
        struct Adapter* adapter;
        struct stack_src_t src;
        macaddress_t source_mac;
        macaddress_t router_mac_ipv4;
        macaddress_t router_mac_ipv6;
        ipv4address_t router_ip;
        int link_type;              /* libpcap definitions */
        unsigned char my_mac_count; /*is there a MAC address? */
        unsigned vlan_id;
        unsigned is_vlan : 1;
        unsigned is_usable : 1;
    } nic;
    struct {
        struct PayloadsUDP* udp;
        struct PayloadsUDP* oproto;
    } payloads;
    struct queue_stack* stack;
    struct TemplateSet tmplset[1];
    uint64_t seed;
    unsigned src_ipv4;
    unsigned src_ipv4_mask;
    unsigned src_port;
    unsigned src_port_mask;
    ipv6address src_ipv6;
    ipv6address src_ipv6_mask;

    size_t thread_handle_send;
    size_t thread_handle_recv;
    struct HostScanResult* result;
    struct ARPItem* arp_table;
    unsigned arp_table_count;
};

void add_port_result(struct HostScanResult* result, unsigned int type, unsigned int port);

struct HostScanResult* lookup_or_new_host_scan_result(struct HostScanTask* task, ipaddress ip);

struct HostScanTask* init_host_scan_task(const char* targets, const char* port, unsigned is_arp,
                                         unsigned is_icmp, const char* ifname,
                                         struct ARPItem* arp_table, unsigned arp_table_size);

void destory_host_scan_task(struct HostScanTask* task);

void start_host_scan_task(struct HostScanTask* task);

void join_host_scan_task(struct HostScanTask* task);

unsigned is_private_address(ipaddress ip);

unsigned lookup_arp_table(struct HostScanTask* task, ipaddress address, macaddress_t* mac);

struct ARPItem* resolve_mac_address(const char* host, unsigned* item, unsigned timeout_second);

void masscan_init();

struct PCAPSocket {
    unsigned int shutdown;
    char ifname[256];
    struct Adapter* adapter;
    ipaddress my_ip;
    ipaddress them_ip;
    macaddress_t my_mac;
    macaddress_t them_mac;
    ipaddress router_ip;
    macaddress_t router_mac;
};

typedef struct PCAPSocket* HSocket;

HSocket raw_open_socket(ipaddress dst, const char* ifname);

void raw_close_socket(HSocket handle);

int raw_socket_send(HSocket handle, const unsigned char* data, unsigned int data_size);

int raw_socket_recv(HSocket handle, const unsigned char** pkt, unsigned int* pkt_size);
#endif /* hostscan_h */
