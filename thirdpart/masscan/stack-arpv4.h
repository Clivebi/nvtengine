#ifndef STACK_ARP_H
#define STACK_ARP_H
struct Adapter;
#include "stack-queue.h"
#include "massip-addr.h"

/**
 * Response to an ARP request for our IP address.
 *
 * @param my_ip
 *      My IP address
 * @param my_mac
 *      My Ethernet MAC address that matches this IP address.
 * @param px
 *      The incoming ARP request
 * @param length
 *      The length of the incoming ARP request.
 * @param packet_buffers
 *      Free packet buffers I can use to format the request
 * @param transmit_queue
 *      I put the formatted response onto this queue for later
 *      transmission by a transmit thread.
 */
int stack_arp_incoming_request(struct queue_stack *stack,
        ipv4address_t my_ip, macaddress_t my_mac,
        const unsigned char *px, unsigned length);

/**
 * Send an ARP request in order to resolve an IPv4 address into a
 * MAC address. Usually done in order to find the local router's 
 * MAC address when given the IPv4 address of the router.
 */
int stack_arp_resolve(struct Adapter *adapter,
    ipv4address_t my_ipv4, macaddress_t my_mac_address,
    ipv4address_t your_ipv4, macaddress_t *your_mac_address,unsigned *shutdown);

struct ARP_IncomingRequest {
    unsigned is_valid;
    unsigned opcode;
    unsigned hardware_type;
    unsigned protocol_type;
    unsigned hardware_length;
    unsigned protocol_length;
    unsigned ip_src;
    unsigned ip_dst;
    const unsigned char* mac_src;
    const unsigned char* mac_dst;
};


void proto_arp_parse(struct ARP_IncomingRequest* arp, const unsigned char px[], unsigned offset,
                     unsigned max);

#endif
