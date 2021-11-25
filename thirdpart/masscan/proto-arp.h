#ifndef PROTO_ARP_H
#define PROTO_ARP_H
#include <time.h>
struct HostScanTask;
struct PreprocessedInfo;


void
handle_arp_response(struct HostScanTask* task, time_t timestamp, const unsigned char *px,
                    unsigned length, struct PreprocessedInfo *parsed);
#endif
