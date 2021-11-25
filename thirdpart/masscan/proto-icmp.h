#ifndef PROTO_ICMP_H
#define PROTO_ICMP_H
#include <time.h>
#include <stdint.h>
struct PreprocessedInfo;
struct HostScanTask;

void handle_icmp(struct HostScanTask *out, time_t timestamp,
        const unsigned char *px, unsigned length, 
        struct PreprocessedInfo *parsed,
        uint64_t entropy);

#endif
