/*
 
 main
 
 This includes:
 
 * main()
 * transmit_thread() - transmits probe packets
 * receive_thread() - receives response packets
 
 You'll be wanting to study the transmit/receive threads, because that's
 where all the action is.
 
 This is the lynch-pin of the entire program, so it includes a heckuva lot
 of headers, and the functions have a lot of local variables. I'm trying
 to make this file relative "flat" this way so that everything is visible.
 */
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crypto-base64.h" /* base64 encode/decode */
#include "in-binary.h"     /* convert binary output to XML/JSON */
#include "logger.h"        /* adjust with -v command-line opt */
#include "logger.h"
#include "main-dedup.h"   /* ignore duplicate responses */
#include "main-globals.h" /* all the global variables in the program */
#include "main-ptrace.h"  /* for nmap --packet-trace feature */
#include "main-readrange.h"
#include "main-status.h"    /* printf() regular status updates */
#include "main-throttle.h"  /* rate limit */
#include "masscan-status.h" /* open or closed */
#include "masscan-version.h"
#include "masscan.h"
#include "massip-parse.h"
#include "massip-port.h"
#include "misc-rstfilter.h"
#include "output.h" /* for outputting results */
#include "pixie-backtrace.h"
#include "pixie-threads.h"    /* portable threads */
#include "pixie-timer.h"      /* portable time functions */
#include "proto-arp.h"        /* for responding to ARP requests */
#include "proto-banner1.h"    /* for snatching banners from systems */
#include "proto-coap.h"       /* CoAP selftest */
#include "proto-icmp.h"       /* handle ICMP responses */
#include "proto-ntp.h"        /* parse NTP responses */
#include "proto-oproto.h"     /* Other protocols on top of IP */
#include "proto-preprocess.h" /* quick parse of packets */
#include "proto-sctp.h"
#include "proto-snmp.h" /* parse SNMP responses */
#include "proto-tcp.h"  /* for TCP/IP connection table */
#include "proto-udp.h"  /* handle UDP responses */
#include "proto-x509.h"
#include "proto-zeroaccess.h"
#include "rand-blackrock.h" /* the BlackRock shuffling func */
#include "rand-lcg.h"       /* the LCG randomization func */
#include "rawsock-adapter.h"
#include "rawsock-pcapfile.h" /* for saving pcap files w/ raw packets */
#include "rawsock.h"          /* API on top of Linux, Windows, Mac OS X*/
#include "rawsock.h"
#include "read-service-probes.h"
#include "rte-ring.h" /* producer/consumer ring buffer */
#include "scripting.h"
#include "siphash24.h"
#include "smack.h"       /* Aho-corasick state-machine pattern-matcher */
#include "stack-arpv4.h" /* Handle ARP resolution and requests */
#include "stack-arpv4.h"
#include "stack-ndpv6.h" /* IPv6 Neighbor Discovery Protocol */
#include "stack-ndpv6.h"
#include "stub-pcap-dlt.h"
#include "stub-pcap.h" /* dynamically load libpcap library */
#include "stub-pcap.h"
#include "stub-pfring.h"
#include "syn-cookie.h"     /* for SYN-cookies on send */
#include "templ-payloads.h" /* UDP packet payloads */
#include "templ-pkt.h"      /* packet template, that we use to send */
#include "util-checksum.h"
#include "util-malloc.h"
#include "vulncheck.h" /* checking vulns like monlist, poodle, heartblee */

#if defined(_WIN32)
#include <WinSock.h>
#if defined(_MSC_VER)
#pragma comment(lib, "Ws2_32.lib")
#endif
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "hostscan.h"

time_t global_now;

void add_port_result(struct HostScanResult* result, unsigned int type, unsigned int port) {
    result->list = REALLOCARRAY(result->list, result->size + 1, sizeof(struct PortResult));
    result->list[result->size].type = type;
    result->list[result->size].port = port;
    result->size++;
}

/***************************************************************************
 * Initialize the network adapter.
 *
 * This requires finding things like our IP address, MAC address, and router
 * MAC address. The user could configure these things manually instead.
 *
 * Note that we don't update the "static" configuration with the discovered
 * values, but instead return them as the "running" configuration. That's
 * so if we pause and resume a scan, auto discovered values don't get saved
 * in the configuration file.
 ***************************************************************************/
static int initialize_task_adapter(struct HostScanTask* task) {
    char* ifname;
    unsigned adapter_ip = 0;
    unsigned is_usable_ipv4 = !massip_has_ipv4_targets(&task->targets);
    unsigned is_usable_ipv6 = !massip_has_ipv6_targets(&task->targets);
    ipaddress_formatted_t fmt;

    /*
     * ADAPTER/NETWORK-INTERFACE
     *
     * If no network interface was configured, we need to go hunt down
     * the best Interface to use. We do this by choosing the first
     * interface with a "default route" (aka. "gateway") defined
     */
    if (task->nic.ifname[0])
        ifname = task->nic.ifname;
    else {
        /* no adapter specified, so find a default one */
        int err;
        err = rawsock_get_default_interface(task->nic.ifname, 256);
        if (err || task->nic.ifname[0] == '\0') {
            LOG(0, "[-] FAIL: could not determine default interface\n");
            LOG(0, "    [hint] try \"--interface ethX\"\n");
            return -1;
        }
        ifname = task->nic.ifname;
    }
    LOG(1, "[+] interface = %s\n", ifname);

    /*
     * START ADAPTER
     *
     * Once we've figured out which adapter to use, we now need to
     * turn it on.
     */
    task->nic.adapter = rawsock_init_adapter(ifname, 0, true, 0, 0, NULL, false, 0);
    if (task->nic.adapter == 0) {
        LOG(0, "[-] if:%s:init: failed\n", ifname);
        return -1;
    }
    task->nic.link_type = task->nic.adapter->link_type;
    LOG(1, "[+] interface-type = %u\n", task->nic.link_type);
    rawsock_ignore_transmits(task->nic.adapter, ifname);

    /*
     * MAC ADDRESS
     *
     * This is the address we send packets from. It actually doesn't really
     * matter what this address is, but to be a "responsible" citizen we
     * try to use the hardware address in the network card.
     */
    if (task->nic.link_type == PCAP_DLT_NULL) {
        LOG(1, "[+] source-mac = %s\n", "none");
    } else if (task->nic.link_type == PCAP_DLT_RAW) {
        LOG(1, "[+] source-mac = %s\n", "none");
    } else {
        if (task->nic.my_mac_count == 0) {
            if (macaddress_is_zero(task->nic.source_mac)) {
                rawsock_get_adapter_mac(ifname, task->nic.source_mac.addr);
            }
            /* If still zero, then print error message */
            if (macaddress_is_zero(task->nic.source_mac)) {
                fprintf(stderr,
                        "[-] FAIL: failed to detect MAC address of interface:"
                        " \"%s\"\n",
                        ifname);
                fprintf(stderr,
                        " [hint] try something like "
                        "\"--source-mac 00-11-22-33-44-55\"\n");
                return -1;
            }
        }

        fmt = macaddress_fmt(task->nic.source_mac);
        LOG(1, "[+] source-mac = %s\n", fmt.string);
    }

    /*
     * IPv4 ADDRESS
     *
     * We need to figure out that IP address to send packets from. This
     * is done by querying the adapter (or configured by user). If the
     * adapter doesn't have one, then the user must configure one.
     */
    if (massip_has_ipv4_targets(&task->targets)) {
        adapter_ip = task->nic.src.ipv4.first;
        if (adapter_ip == 0) {
            adapter_ip = rawsock_get_adapter_ip(ifname);
            task->nic.src.ipv4.first = adapter_ip;
            task->nic.src.ipv4.last = adapter_ip;
            task->nic.src.ipv4.range = 1;
        }
        if (adapter_ip == 0) {
            /* We appear to have IPv4 targets, yet we cannot find an adapter
             * to use for those targets. We are having trouble querying the
             * operating system stack. */
            LOG(0, "[-] FAIL: failed to detect IP of interface \"%s\"\n", ifname);
            LOG(0, "    [hint] did you spell the name correctly?\n");
            LOG(0,
                "    [hint] if it has no IP address, manually set with something like "
                "\"--source-ip 198.51.100.17\"\n");
            if (massip_has_ipv4_targets(&task->targets)) {
                return -1;
            }
        }

        fmt = ipv4address_fmt(adapter_ip);
        LOG(1, "[+] source-ip = %s\n", fmt.string);

        if (adapter_ip != 0) is_usable_ipv4 = 1;

        /*
         * ROUTER MAC ADDRESS
         *
         * NOTE: this is one of the least understood aspects of the code. We must
         * send packets to the local router, which means the MAC address (not
         * IP address) of the router.
         *
         * Note: in order to ARP the router, we need to first enable the libpcap
         * code above.
         */
        if (task->nic.link_type == PCAP_DLT_NULL) {
            /* If it's a VPN tunnel, then there is no Ethernet MAC address */
            LOG(1, "[+] router-mac-ipv4 = %s\n", "implicit");
        } else if (task->nic.link_type == PCAP_DLT_RAW) {
            /* If it's a VPN tunnel, then there is no Ethernet MAC address */
            LOG(1, "[+] router-mac-ipv4 = %s\n", "implicit");
        } else if (macaddress_is_zero(task->nic.router_mac_ipv4)) {
            ipv4address_t router_ipv4 = task->nic.router_ip;
            int err = 0;

            LOG(2, "[+] if(%s): looking for default gateway\n", ifname);
            if (router_ipv4 == 0) err = rawsock_get_default_gateway(ifname, &router_ipv4);
            if (err == 0) {
                fmt = ipv4address_fmt(router_ipv4);
                LOG(1, "[+] router-ip = %s\n", fmt.string);
                LOG(2, "[+] if(%s):arp: resolving IPv4 address\n", ifname);

                stack_arp_resolve(task->nic.adapter, adapter_ip, task->nic.source_mac, router_ipv4,
                                  &task->nic.router_mac_ipv4, &task->shutdown);
            }
            task->nic.router_ip = router_ipv4;

            fmt = macaddress_fmt(task->nic.router_mac_ipv4);
            LOG(1, "[+] router-mac-ipv4 = %s\n", fmt.string);
            if (macaddress_is_zero(task->nic.router_mac_ipv4)) {
                fmt = ipv4address_fmt(task->nic.router_ip);
                LOG(0, "[-] FAIL: ARP timed-out resolving MAC address for router %s: \"%s\"\n",
                    ifname, fmt.string);
                LOG(0, "    [hint] try \"--router ip 192.0.2.1\" to specify different router\n");
                LOG(0, "    [hint] try \"--router-mac 66-55-44-33-22-11\" instead to bypass ARP\n");
                LOG(0, "    [hint] try \"--interface eth0\" to change interface\n");
                return -1;
            }
        }
    }

    /*
     * IPv6 ADDRESS
     *
     * We need to figure out that IPv6 address to send packets from. This
     * is done by querying the adapter (or configured by user). If the
     * adapter doesn't have one, then the user must configure one.
     */
    if (massip_has_ipv6_targets(&task->targets)) {
        ipv6address adapter_ipv6 = task->nic.src.ipv6.first;
        if (ipv6address_is_zero(adapter_ipv6)) {
            adapter_ipv6 = rawsock_get_adapter_ipv6(ifname);
            task->nic.src.ipv6.first = adapter_ipv6;
            task->nic.src.ipv6.last = adapter_ipv6;
            task->nic.src.ipv6.range = 1;
        }
        if (ipv6address_is_zero(adapter_ipv6)) {
            fprintf(stderr, "[-] FAIL: failed to detect IPv6 address of interface \"%s\"\n",
                    ifname);
            fprintf(stderr, "    [hint] did you spell the name correctly?\n");
            fprintf(stderr,
                    "    [hint] if it has no IP address, manually set with something like "
                    "\"--source-ip 2001:3b8::1234\"\n");
            return -1;
        }
        fmt = ipv6address_fmt(adapter_ipv6);
        LOG(1, "[+] source-ip = [%s]\n", fmt.string);
        is_usable_ipv6 = 1;

        /*
         * ROUTER MAC ADDRESS
         */
        if (macaddress_is_zero(task->nic.router_mac_ipv6)) {
            /* [synchronous]
             * Wait for router neighbor notification. This may take
             * some time */
            stack_ndpv6_resolve(task->nic.adapter, adapter_ipv6, task->nic.source_mac,
                                &task->shutdown, &task->nic.router_mac_ipv6);
        }

        fmt = macaddress_fmt(task->nic.router_mac_ipv6);
        LOG(1, "[+] router-mac-ipv6 = %s\n", fmt.string);
        if (macaddress_is_zero(task->nic.router_mac_ipv6)) {
            fmt = ipv4address_fmt(task->nic.router_ip);
            LOG(0, "[-] FAIL: NDP timed-out resolving MAC address for router %s: \"%s\"\n", ifname,
                fmt.string);
            LOG(0,
                "    [hint] try \"--router-mac-ipv6 66-55-44-33-22-11\" instead to bypass ARP\n");
            LOG(0, "    [hint] try \"--interface eth0\" to change interface\n");
            return -1;
        }
    }

    task->nic.is_usable = (is_usable_ipv4 & is_usable_ipv6);

    LOG(2, "[+] if(%s): initialization done.\n", ifname);
    return 0;
}

struct HostScanTask* init_host_scan_task(const char* targets, const char* port_list,
                                         unsigned is_arp, unsigned is_icmp, const char* ifname,
                                         struct ARPItem* arp_table, unsigned arp_table_size,
                                         unsigned rate) {
    time_t now = time(0);
    struct HostScanTask* task = MALLOC(sizeof(struct HostScanTask));
    memset(task, 0, sizeof(struct HostScanTask));
    massip_add_target_string(&task->targets, targets);
    if (is_arp) {
        task->arp = 1;
        rangelist_add_range(&task->targets.ports, Templ_ARP, Templ_ARP);
    } else if (is_icmp) {
        task->icmp = 1;
        rangelist_add_range(&task->targets.ports, Templ_ICMP_echo, Templ_ICMP_echo);
    } else {
        massip_add_port_string(&task->targets, port_list, 0);
    }
    task->retries = 1;
    task->seed = get_entropy();
    task->count_ipv4 = rangelist_count(&task->targets.ipv4);
    task->count_ipv6 = range6list_count(&task->targets.ipv6).lo;
    task->range = task->count_ipv4 * rangelist_count(&task->targets.ports) +
                  task->count_ipv6 * rangelist_count(&task->targets.ports);
    task->range_ipv6 = task->count_ipv6 * rangelist_count(&task->targets.ports);
    task->total_count = (task->range + task->range_ipv6) * task->retries;
    task->send_count = 0;
    task->rate = rate;
    if (task->total_count == 0) {
        free(task);
        return NULL;
    }
    strcpy_s(task->nic.ifname, 256, ifname);
    if (initialize_task_adapter(task) || !task->nic.is_usable) {
        free(task);
        return NULL;
    }

    task->payloads.udp = payloads_udp_create();
    task->payloads.oproto = payloads_oproto_create();

    template_packet_init(&task->tmplset, task->nic.source_mac, task->nic.router_mac_ipv4,
                         task->nic.router_mac_ipv6, task->payloads.udp, task->payloads.oproto,
                         stack_if_datalink(task->nic.adapter), task->seed);

    if (task->nic.src.port.range == 0) {
        unsigned port = 40000 + now % 20000;
        task->nic.src.port.first = port;
        task->nic.src.port.last = port;
        task->nic.src.port.range = 1;
    }

    task->stack = stack_create(task->nic.source_mac, &task->nic.src);
    static ipv6address mask = {~0ULL, ~0ULL};

    task->src_ipv4 = task->nic.src.ipv4.first;
    task->src_ipv4_mask = task->nic.src.ipv4.last - task->nic.src.ipv4.first;

    task->src_port = task->nic.src.port.first;
    task->src_port_mask = task->nic.src.port.last - task->nic.src.port.first;

    task->src_ipv6 = task->nic.src.ipv6.first;

    if (arp_table && arp_table_size) {
        task->arp_table = MALLOC(sizeof(struct ARPItem) * arp_table_size);
        memcpy(task->arp_table, arp_table, sizeof(struct ARPItem) * arp_table_size);
        task->arp_table_count = arp_table_size;
    }

    /* TODO: currently supports only a single address. This needs to
     * be fixed to support a list of addresses */
    task->src_ipv6_mask = mask;
    return task;
}

void destory_host_scan_task(struct HostScanTask* task) {
    struct HostScanResult *seek, *temp;
    join_host_scan_task(task);
    massip_destory(&task->targets);
    rawsock_close_adapter(task->nic.adapter);
    seek = task->result;
    while (seek != NULL) {
        temp = seek;
        seek = seek->next;
        free(temp->list);
        free(temp);
    }
    if (task->arp_table != NULL) {
        free(task->arp_table);
    }
    payloads_udp_destroy(task->payloads.udp);
    payloads_udp_destroy(task->payloads.oproto);
    destory_TemplateSet(&task->tmplset);
    free(task);
}

static void recv_thread(struct HostScanTask* task);
static void send_thread(struct HostScanTask* task);
void start_host_scan_task(struct HostScanTask* task) {
    if (task->thread_handle_send != 0 || task->thread_handle_recv != 0) {
        return;
    }
    task->thread_handle_recv = pixie_begin_thread((void (*)(void*))recv_thread, 0, task);
    task->thread_handle_send = pixie_begin_thread((void (*)(void*))send_thread, 0, task);
}

void join_host_scan_task(struct HostScanTask* task) {
    task->shutdown = true;
    if (task->thread_handle_send != 0) {
        pixie_thread_join(task->thread_handle_recv);
        pixie_thread_join(task->thread_handle_send);
    }
    task->thread_handle_recv = 0;
    task->thread_handle_send = 0;
    if(task->stack){
        stack_destory(task->stack);
        task->stack = NULL;
    }
}

static unsigned is_ipv6_multicast(ipaddress ip_me) {
    /* If this is an IPv6 multicast packet, one sent to the IPv6
     * address with a prefix of FF02::/16 */
    return ip_me.version == 6 && (ip_me.ipv6.hi >> 48ULL) == 0xFF02;
}

unsigned is_private_address(ipaddress ip) {
    if (ip.version == 4) {
        unsigned char* ptr = (unsigned char*)&ip.ipv4;
        if (ptr[3] == 172 && ((unsigned int)ptr[2] & 0xfe) == 16) {
            return 1;
        }
        if (ptr[3] == 192 && ptr[2] == 168) {
            return 1;
        }
    } else {
        return (ip.ipv6.lo & 0xfe) == 0xfc;
    }
    return 0;
}
unsigned lookup_arp_table(struct HostScanTask* task, ipaddress address, macaddress_t* mac) {
    for (size_t i = 0; i < task->arp_table_count; i++) {
        if (ipaddress_is_equal(address, task->arp_table[i].ip)) {
            *mac = task->arp_table[i].mac;
            return 1;
        }
    }
    return 0;
}

struct HostScanResult* lookup_or_new_host_scan_result(struct HostScanTask* task, ipaddress ip) {
    struct HostScanResult* seek = task->result;
    while (seek != NULL) {
        if (ipaddress_is_equal(seek->address, ip)) {
            return seek;
        }
        seek = seek->next;
    }
    seek = MALLOC(sizeof(struct HostScanResult));
    memset(seek, 0, sizeof(struct HostScanResult));
    seek->address = ip;
    seek->next = task->result;
    task->result = seek;
    return seek;
}

static void recv_thread(struct HostScanTask* task) {
    struct DedupTable* dedup = NULL;
    dedup = dedup_create();
    LOG(2, "[+] THREAD: recv: starting main loop\n");
    while (!task->shutdown) {
        int status;
        unsigned length;
        unsigned secs;
        unsigned usecs;
        const unsigned char* px;
        int err;
        unsigned x;
        struct PreprocessedInfo parsed;
        ipaddress ip_me;
        unsigned port_me;
        ipaddress ip_them;
        unsigned port_them;
        unsigned seqno_me;
        unsigned seqno_them;
        unsigned cookie;

        if (task->range == 0 && task->range_ipv6 == 0) {
            break;
        }

        /*
         * RECEIVE
         *
         * This is the boring part of actually receiving a packet
         */
        err = rawsock_recv_packet(task->nic.adapter, &length, &secs, &usecs, &task->shutdown, &px);
        if (err != 0) {
            continue;
        }
        if (length > 1514) continue;

        /*
         * "Preprocess" the response packet. This means to go through and
         * figure out where the TCP/IP headers are and the locations of
         * some fields, like IP address and port numbers.
         */
        x = preprocess_frame(px, length, task->nic.link_type, &parsed);
        if (!x) continue; /* corrupt packet */
        ip_me = parsed.dst_ip;
        ip_them = parsed.src_ip;
        port_me = parsed.port_dst;
        port_them = parsed.port_src;
        seqno_them = TCP_SEQNO(px, parsed.transport_offset);
        seqno_me = TCP_ACKNO(px, parsed.transport_offset);

        assert(ip_me.version != 0);
        assert(ip_them.version != 0);

        switch (parsed.ip_protocol) {
        case 132: /* SCTP */
            cookie = syn_cookie(ip_them, port_them | (Proto_SCTP << 16), ip_me, port_me,
                                task->seed) &
                     0xFFFFFFFF;
            break;
        default:
            cookie = syn_cookie(ip_them, port_them, ip_me, port_me, task->seed) & 0xFFFFFFFF;
        }

        /* verify: my IP address */
        if (!is_my_ip(task->stack->src, ip_me)) {
            /* NDP Neighbor Solicitations don't come to our IP address, but to
             * a multicast address */
            if (is_ipv6_multicast(ip_me)) {
                if (parsed.found == FOUND_NDPv6 && parsed.opcode == 135) {
                    stack_ndpv6_incoming_request(task->stack, &parsed, px, length);
                }
            }
            continue;
        }

        /*
         * Handle non-TCP protocols
         */
        switch (parsed.found) {
        case FOUND_NDPv6:
            switch (parsed.opcode) {
            case 133: /* Router Solicitation */
                /* Ignore router solicitations, since we aren't a router */
                continue;
            case 134: /* Router advertisement */
                /* TODO: We need to process router advertisements while scanning
                         * so that we can print warning messages if router information
                         * changes while scanning. */
                continue;
            case 135: /* Neighbor Solicitation */
                /* When responses come back from our scans, the router will send us
                         * these packets. We need to respond to them, so that the router
                         * can then forward the packets to us. If we don't respond, we'll
                         * get no responses. */
                stack_ndpv6_incoming_request(task->stack, &parsed, px, length);
                continue;
            case 136: /* Neighbor Advertisement */
                /* TODO: If doing an --ndpscan, the scanner subsystem needs to deal
                         * with these */
                continue;
            case 137: /* Redirect */
                /* We ignore these, since we really don't have the capability to send
                         * packets to one router for some destinations and to another router
                         * for other destinations */
                continue;
            default:
                break;
            }
            continue;
        case FOUND_ARP:
            //LOGip(2, ip_them, 0, "-> ARP [%u] \n", px[parsed.found_offset]);

            switch (parsed.opcode) {
            case 1: /* request */
                /* This function will transmit a "reply" to somebody's ARP request
                         * for our IP address (as part of our user-mode TCP/IP).
                         * Since we completely bypass the TCP/IP stack, we  have to handle ARPs
                         * ourself, or the router will lose track of us.*/
                stack_arp_incoming_request(task->stack, ip_me.ipv4, task->nic.source_mac, px,
                                           length);
                break;
            case 2: /* response */
                /* This is for "arp scan" mode, where we are ARPing targets rather
                         * than port scanning them */

                /* If this response isn't in our range, then ignore it */
                if (!rangelist_is_contains(&task->targets.ipv4, ip_them.ipv4)) {
                    break;
                }

                /* Ignore duplicates */
                if (dedup_is_duplicate(dedup, ip_them, 0, ip_me, 0)) {
                    break;
                }
                task->count.arp++;
                handle_arp_response(task, secs, px, length, &parsed);
                if (task->count.arp == task->range + task->range_ipv6 && task->arp) {
                    task->recv_done = 1;
                }
                break;
            }
            continue;
        case FOUND_UDP:
        case FOUND_DNS:
            if (!is_my_port(&task->nic.src, port_me)) continue;
            handle_udp(task, secs, px, length, &parsed, task->seed);
            continue;
        case FOUND_ICMP:
            handle_icmp(task, secs, px, length, &parsed, task->seed);
            continue;
        case FOUND_SCTP:
            handle_sctp(task, secs, px, length, cookie, &parsed, task->seed);
            break;
        case FOUND_OPROTO: /* other IP proto */
            //handle_oproto(out, secs, px, length, &parsed, entropy);
            break;
        case FOUND_TCP:
            /* fall down to below */
            break;
        default:
            continue;
        }

        /* verify: my port number */
        if (!is_my_port(task->stack->src, port_me)) continue;

        if (TCP_IS_SYNACK(px, parsed.transport_offset) || TCP_IS_RST(px, parsed.transport_offset)) {
            /* figure out the status */
            status = PortStatus_Unknown;
            if (TCP_IS_SYNACK(px, parsed.transport_offset)) status = PortStatus_Open;
            if (TCP_IS_RST(px, parsed.transport_offset)) {
                status = PortStatus_Closed;
            }

            /* verify: syn-cookies */
            if (cookie != seqno_me - 1) {
                ipaddress_formatted_t fmt = ipaddress_fmt(ip_them);
                LOG(5, "%s - bad cookie: ackno=0x%08x expected=0x%08x\n", fmt.string, seqno_me - 1,
                    cookie);
                continue;
            }

            /* verify: ignore duplicates */
            if (dedup_is_duplicate(dedup, ip_them, port_them, ip_me, port_me)) continue;
            /*
             * This is where we do the output
             */
            if (status == PortStatus_Open) {
                add_port_result(lookup_or_new_host_scan_result(task, ip_them), TCP_UNKNOWN,
                                port_them);
            }
        }
    }

    //LOG(1, "[+] exiting receive thread #%u                    \n", parms->nic_index);

    /*
     * cleanup
     */
    dedup_destroy(dedup);
}

static void send_thread(struct HostScanTask* task) {
    uint64_t i;
    uint64_t start = 0;
    uint64_t end = task->total_count;
    unsigned int progress = 0;
    unsigned int preprogress = 0;
    uint64_t packets_sent = 0;
    struct BlackRock blackrock;
    uint64_t repeats = 0; /* --infinite repeats */
    struct Throttler throttler;

    LOG(1, "[+] starting transmit thread");
    LOG(1, "THREAD: xmit: starting main loop: [%llu..%llu] rate=%d\n", start, end,task->rate);
    throttler_start(&throttler, (double)task->rate);
    blackrock_init(&blackrock, task->range, task->seed, 14);
    while (!task->shutdown) {
        if (task->range == 0 && task->range_ipv6 == 0) {
            break;
        }
        if (task->send_done) {
            uint64_t batch_size;
            batch_size = throttler_next_batch(&throttler, packets_sent);

            /*
             * Transmit packets from other thread, when doing --banners. This
             * takes priority over sending SYN packets. If there is so much
             * activity grabbing banners that we cannot transmit more SYN packets,
             * then "batch_size" will get decremented to zero, and we won't be
             * able to transmit SYN packets.
             */
            stack_flush_packets(task->stack, task->nic.adapter, &packets_sent, &batch_size);
            pixie_usleep(700);
            continue;
        }
        for (i = start; i < end;) {
            uint64_t batch_size;

            /*
             * Do a batch of many packets at a time. That because per-packet
             * throttling is expensive at 10-million pps, so we reduce the
             * per-packet cost by doing batches. At slower rates, the batch
             * size will always be one. (--max-rate)
             */
            batch_size = throttler_next_batch(&throttler, packets_sent);
            progress =(unsigned int)(i * 100 / end);
            if (progress != preprogress) {
                LOG(2, "current send progress:%ld packets:%llu batch_size:%llu\n", progress,packets_sent,batch_size);
                preprogress = progress;
            }
            /*
             * Transmit packets from other thread, when doing --banners. This
             * takes priority over sending SYN packets. If there is so much
             * activity grabbing banners that we cannot transmit more SYN packets,
             * then "batch_size" will get decremented to zero, and we won't be
             * able to transmit SYN packets.
             */
            stack_flush_packets(task->stack, task->nic.adapter, &packets_sent, &batch_size);

            /*
             * Transmit a bunch of packets. At any rate slower than 100,000
             * packets/second, the 'batch_size' is likely to be 1. At higher
             * rates, we can't afford to throttle on a per-packet basis and
             * instead throttle on a per-batch basis. In other words, throttle
             * based on 2-at-a-time, 3-at-time, and so on, with the batch
             * size increasing as the packet rate increases. This gives us
             * very precise packet-timing for low rates below 100,000 pps,
             * while not incurring the overhead for high packet rates.
             */
            while (batch_size && i < end) {
                uint64_t xXx;
                uint64_t cookie;

                /*
                 * RANDOMIZE THE TARGET:
                 *  This is kinda a tricky bit that picks a random IP and port
                 *  number in order to scan. We monotonically increment the
                 *  index 'i' from [0..range]. We then shuffle (randomly transmog)
                 *  that index into some other, but unique/1-to-1, number in the
                 *  same range. That way we visit all targets, but in a random
                 *  order. Then, once we've shuffled the index, we "pick" the
                 *  IP address and port that the index refers to.
                 */
                xXx = (i + task->rate);
                if (task->rate > task->range)
                    xXx %= task->range;
                else
                    while (xXx >= task->range) xXx -= task->range;
                xXx = blackrock_shuffle(&blackrock, xXx);

                if (xXx < task->range_ipv6) {
                    ipv6address ip_them;
                    unsigned port_them;
                    ipv6address ip_me;
                    unsigned port_me;

                    ip_them = range6list_pick(&task->targets.ipv6, xXx % task->count_ipv6);
                    port_them = rangelist_pick(&task->targets.ports, xXx / task->count_ipv6);

                    ip_me = task->src_ipv6;
                    port_me = task->src_port;

                    cookie = syn_cookie_ipv6(ip_them, port_them, ip_me, port_me, task->seed);

                    rawsock_send_probe_ipv6(task, task->nic.adapter, ip_them, port_them, ip_me,
                                            port_me, (unsigned)cookie,
                                            !batch_size, /* flush queue on last packet in batch */
                                            &task->tmplset);

                    /* Our index selects an IPv6 target */
                } else {
                    /* Our index selects an IPv4 target. In other words, low numbers
                     * index into the IPv6 ranges, and high numbers index into the
                     * IPv4 ranges. */
                    ipv4address ip_them;
                    ipv4address port_them;
                    unsigned ip_me;
                    unsigned port_me;

                    xXx -= task->range_ipv6;

                    ip_them = rangelist_pick(&task->targets.ipv4, xXx % task->count_ipv4);
                    port_them = rangelist_pick(&task->targets.ports, xXx / task->count_ipv4);

                    /*
                     * SYN-COOKIE LOGIC
                     *  Figure out the source IP/port, and the SYN cookie
                     */
                    if (task->src_ipv4_mask > 1 || task->src_port_mask > 1) {
                        uint64_t ck = syn_cookie_ipv4(
                                (unsigned)(i + repeats), (unsigned)((i + repeats) >> 32),
                                (unsigned)xXx, (unsigned)(xXx >> 32), task->seed);
                        port_me = task->src_port + (ck & task->src_port_mask);
                        ip_me = task->src_ipv4 + ((ck >> 16) & task->src_ipv4_mask);
                    } else {
                        ip_me = task->src_ipv4;
                        port_me = task->src_port;
                    }
                    cookie = syn_cookie_ipv4(ip_them, port_them, ip_me, port_me, task->seed);

                    /*
                     * SEND THE PROBE
                     *  This is sorta the entire point of the program, but little
                     *  exciting happens here. The thing to note that this may
                     *  be a "raw" transmit that bypasses the kernel, meaning
                     *  we can call this function millions of times a second.
                     */
                    rawsock_send_probe_ipv4(task, task->nic.adapter, ip_them, port_them, ip_me,
                                            port_me, (unsigned)cookie,
                                            !batch_size, /* flush queue on last packet in batch */
                                            &task->tmplset);
                }

                batch_size--;
                packets_sent++;
                i++;

            } /* end of batch */
        }
        rawsock_send_packet(task->nic.adapter, NULL, 0, true);
        task->send_done = 1;
        pixie_usleep(700);
    }
    rawsock_flush(task->nic.adapter);
    LOG(1, "[+] exiting transmit thread");
}

struct ARPItem* resolve_mac_address(const char* host, unsigned* item, unsigned timeout_second) {
    ipaddress address;
    unsigned count = 0;
    struct ARPItem* arp_array = NULL;
    address.version = 4;
    struct HostScanResult* seek = NULL;
    struct HostScanTask* task = init_host_scan_task(host, "", true, false, "", NULL, 0, 50);
    start_host_scan_task(task);
    while (!task->send_done) {
        pixie_mssleep(250);
    }
    for (unsigned i = 0; i < timeout_second * 1000 / 250; i++) {
        if (task->recv_done) {
            break;
        }
        pixie_mssleep(250);
    }
    task->shutdown = 1;
    join_host_scan_task(task);
    seek = task->result;
    while (seek != NULL) {
        count++;
        seek = seek->next;
    }
    *item = count;
    if (count) {
        arp_array = MALLOC(sizeof(struct ARPItem) * count);
        seek = task->result;
        count = 0;
        while (seek != NULL) {
            arp_array[count].ip = seek->address;
            arp_array[count].mac = seek->mac_address;
            count++;
            seek = seek->next;
        }
    }

    destory_host_scan_task(task);
    return arp_array;
}

static void arp_test() {
    unsigned count = 0;
    struct ARPItem* list = resolve_mac_address("192.168.0.100", &count, 5);
    for (unsigned i = 0; i < count; i++) {
        struct ipaddress_formatted mac = macaddress_fmt(list[i].mac);
        struct ipaddress_formatted ip = ipaddress_fmt(list[i].ip);
        printf("%s -->%s\n", ip.string, mac.string);
    }
    free(list);
}

void masscan_init() {
    pcap_init();
    x509_init();
}

HSocket raw_open_socket(ipaddress dst, const char* ifname, const char* bpf_filter) {
    struct PCAPSocket* Handle = NULL;
    int err = 0;
    struct bpf_program filter_prog;

    Handle = (struct PCAPSocket*)malloc(sizeof(struct PCAPSocket));
    if (Handle == NULL) {
        return NULL;
    }
    memset(Handle, 0, sizeof(struct PCAPSocket));
    Handle->shutdown = 0;
    if (ifname != NULL && *ifname) {
        strcpy_s(Handle->ifname, 255, ifname);
    } else {
        err = rawsock_get_default_interface(Handle->ifname, 255);
    }
    if (err != 0) {
        raw_close_socket(Handle);
        return NULL;
    }
    //initialize src address ,src_mac_address,dst_address
    Handle->them_ip = dst;
    if (dst.version == 4) {
        Handle->my_ip.ipv4 = rawsock_get_adapter_ip(Handle->ifname);
    } else {
        Handle->my_ip.ipv6 = rawsock_get_adapter_ipv6(Handle->ifname);
    }
    err = rawsock_get_adapter_mac(Handle->ifname, Handle->my_mac.addr);
    if (err != 0) {
        raw_close_socket(Handle);
        return NULL;
    }
    ////TODO build pcap filter
    Handle->adapter = rawsock_init_adapter(Handle->ifname, 0, 0, 0, 0, bpf_filter, 0, 0);
    if (Handle->adapter == NULL) {
        raw_close_socket(Handle);
        return NULL;
    }
    rawsock_ignore_transmits(Handle->adapter, Handle->ifname);

    if (dst.version == 4) {
        err = rawsock_get_default_gateway(Handle->ifname, &Handle->router_ip.ipv4);
        if (err != 0) {
            raw_close_socket(Handle);
            return NULL;
        }
        stack_arp_resolve(Handle->adapter, Handle->my_ip.ipv4, Handle->my_mac,
                          Handle->router_ip.ipv4, &Handle->router_mac, &Handle->shutdown);
    } else {
        stack_ndpv6_resolve(Handle->adapter, Handle->my_ip.ipv6, Handle->my_mac, &Handle->shutdown,
                            &Handle->router_mac);
    }

    if (macaddress_is_zero(Handle->router_mac)) {
        raw_close_socket(Handle);
        return NULL;
    }

    if (is_private_address(dst)) {
        if (dst.version == 4) {
            stack_arp_resolve(Handle->adapter, Handle->my_ip.ipv4, Handle->my_mac,
                              Handle->them_ip.ipv4, &Handle->them_mac, &Handle->shutdown);
        } else {
            //NDP not implement yet now
        }
        if (macaddress_is_zero(Handle->them_mac)) {
            raw_close_socket(Handle);
            return NULL;
        }
    }
    if (bpf_filter && *bpf_filter && Handle->adapter->pcap != NULL) {
        if (0 != PCAP.compile(Handle->adapter->pcap, &filter_prog, bpf_filter, true, 0xffffffff)) {
            raw_close_socket(Handle);
            return NULL;
        }
        if (0 != PCAP.setfilter(Handle->adapter->pcap, &filter_prog)) {
            PCAP.freecode(&filter_prog);
            raw_close_socket(Handle);
            return NULL;
        }
        PCAP.freecode(&filter_prog);
    }

    return Handle;
}

void raw_close_socket(HSocket handle) {
    if (handle == NULL) {
        return;
    }
    if (handle->adapter != NULL) {
        rawsock_close_adapter(handle->adapter);
        handle->adapter = NULL;
    }
    handle->shutdown = 1;
    free(handle);
}

int raw_socket_send(HSocket handle, const unsigned char* data, unsigned int data_size) {
    unsigned char buffer[1600];
    int ret = 0;
    if (data_size > 1500) {
        return -1;
    }
    if (!macaddress_is_zero(handle->them_mac)) {
        memcpy(buffer, handle->them_mac.addr, 6);
    } else {
        memcpy(buffer, handle->router_mac.addr, 6);
    }
    memcpy(buffer + 6, handle->my_mac.addr, 6);
    if (handle->them_ip.version == 4) {
        buffer[12] = 0x08;
        buffer[13] = 0x00;
    } else {
        buffer[12] = 0x86;
        buffer[13] = 0xdd;
    }
    memcpy(buffer + 14, data, data_size);
    ret = rawsock_send_packet(handle->adapter, buffer, data_size + 14, 1);
    return ret;
}

int raw_socket_recv(HSocket handle, const unsigned char** pkt, unsigned int* pkt_size) {
    unsigned t1, t2;
    return rawsock_recv_packet(handle->adapter, pkt_size, &t1, &t2, &handle->shutdown, pkt);
}

CaptureHandle OpenCapture(const char* ifname, const char* bpf_filter) {
    CaptureHandle Handle = NULL;
    int err = 0;
    struct bpf_program filter_prog;

    Handle = (CaptureHandle)malloc(sizeof(struct CAPTURE_HANDLE));
    if (Handle == NULL) {
        return NULL;
    }
    memset(Handle, 0, sizeof(struct CAPTURE_HANDLE));
    if (ifname != NULL && *ifname) {
        strcpy_s(Handle->ifname, 255, ifname);
    } else {
        err = rawsock_get_default_interface(Handle->ifname, 255);
    }
    if (err != 0) {
        CloseCapture(Handle);
        return NULL;
    }
    Handle->adapter = rawsock_init_adapter(Handle->ifname, 0, 0, 0, 0, bpf_filter, 0, 0);
    if (Handle->adapter == NULL) {
        CloseCapture(Handle);
        return NULL;
    }
    if (bpf_filter && *bpf_filter && Handle->adapter->pcap != NULL) {
        if (0 != PCAP.compile(Handle->adapter->pcap, &filter_prog, bpf_filter, true, 0xffffffff)) {
            CloseCapture(Handle);
            return NULL;
        }
        if (0 != PCAP.setfilter(Handle->adapter->pcap, &filter_prog)) {
            PCAP.freecode(&filter_prog);
            CloseCapture(Handle);
            return NULL;
        }
        PCAP.freecode(&filter_prog);
    }
    return Handle;
}

void CloseCapture(CaptureHandle handle) {
    if (handle == NULL) {
        return;
    }
    if (handle->adapter != NULL) {
        rawsock_close_adapter(handle->adapter);
        handle->adapter = NULL;
    }
    handle->shutdown = 1;
    free(handle);
}

int CapturePacket(CaptureHandle handle, const unsigned char** pkt, unsigned int* pkt_size) {
    unsigned t1, t2;
    return rawsock_recv_packet(handle->adapter, pkt_size, &t1, &t2, &handle->shutdown, pkt);
}
/*
int main(int argc, char* argv[]) {
    struct HostScanResult* seek = NULL;
    pcap_init();
    x509_init();
    unsigned count = 0;
    struct ARPItem* list = resolve_mac_address("192.168.0.1/24", &count, 5);

    struct HostScanTask* task =
            init_host_scan_task("192.168.0.1/24", "80,443", false, false, "", list, count);
    free(list);
    start_host_scan_task(task);
    while (!task->send_done) {
        pixie_mssleep(1000);
    }
    pixie_mssleep(9 * 1000);
    task->shutdown = 1;
    seek = task->result;
    while (seek != NULL) {
        for (size_t i = 0; i < seek->size; i++) {
            struct ipaddress_formatted ip = ipaddress_fmt(seek->address);
            printf("%s %d -->%d\n", ip.string, seek->list[i].type, seek->list[i].port);
        }
        seek = seek->next;
    }
    join_host_sacn_task(task);
    destory_host_scan_task(task);
    return 0;
}
*/