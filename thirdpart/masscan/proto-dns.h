#ifndef PROTO_DNS_H
#define PROTO_DNS_H
#include <time.h>
#include <stdint.h>
struct PreprocessedInfo;
struct HostScanTask;

unsigned handle_dns(struct HostScanTask* task, time_t timestamp,
    const unsigned char *px, unsigned length, struct PreprocessedInfo *parsed, uint64_t entropy);

unsigned dns_set_cookie(unsigned char *px, size_t length, uint64_t seqno);

#endif
