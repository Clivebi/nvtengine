#ifndef PROTO_NETBIOS_H
#define PROTO_NETBIOS_H
#include <time.h>
#include <stdint.h>
struct PreprocessedInfo;
struct HostScanTask;

unsigned handle_nbtstat(struct HostScanTask *out, time_t timestamp,
    const unsigned char *px, unsigned length, 
    struct PreprocessedInfo *parsed,
    uint64_t entropy);

#endif
