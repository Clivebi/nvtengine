extern "C" {
#include "../../../thirdpart/masscan/hostscan.h"
}
#include "../api.hpp"

bool MatchIPResponse(ipaddress src, ipaddress dst, const unsigned char* buf, size_t size) {
    unsigned char src_addr[16] = {0};
    unsigned char dst_addr[16] = {0};
    if (dst.version == 4 && size >= 20) {
        memcpy(dst_addr, buf + 16, 4);
        memcpy(src_addr, buf + 12, 4);
        return memcmp(&src.ipv4, dst_addr, 4) == 0 && memcmp(&dst.ipv4, src_addr, 4) == 0;
    } else if (size > 40) {
        memcpy(dst_addr, buf + 24, 16);
        memcpy(src_addr, buf + 8, 16);
        return memcmp(&src.ipv6, dst_addr, 16) == 0 && memcmp(&dst.ipv6, src_addr, 16) == 0;
    }
    return false;
}

void swapbyte(unsigned char* a, unsigned char* b) {
    unsigned char temp = *b;
    *b = *a;
    *a = temp;
}

unsigned int swapint32(unsigned int src) {
    unsigned int val = src;
    unsigned char* ptr = (unsigned char*)&val;
    swapbyte(ptr, ptr + 3);
    swapbyte(ptr + 1, ptr + 2);
    return val;
}

//TODO add arp cache support?
//packet,bpffilter,timeout,read_answer
Value PcapSend(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    CHECK_PARAMETER_INTEGER(2);
    std::string pkt = GetString(args, 0);
    std::string bpfFilter = GetString(args, 1);
    int timeout = GetInt(args, 2, 5);
    bool readAnswer = args[3].ToBoolean();
    if (pkt.size() == 0) {
        throw RuntimeException("PcapRequest invalid packet");
    }
    int version = (pkt[0] >> 4) & 0xF;
    if ((version == 4 && pkt.size() < 20) || (version == 6 && pkt.size() < 40)) {
        throw RuntimeException("PcapRequest invalid packet");
    }
    ipaddress dst;
    ipaddress src;
    if (version == 4) {
        dst.version = 4;
        src.version = 4;
        memcpy(&dst.ipv4, pkt.c_str() + 16, 4);
        dst.ipv4 = swapint32(dst.ipv4);
        memcpy(&src.ipv4, pkt.c_str() + 12, 4);
        src.ipv4 = swapint32(src.ipv4);
    } else {
        dst.version = 6;
        src.version = 6;
        memcpy(&dst.ipv6, pkt.c_str() + 24, 16);
        memcpy(&src.ipv6, pkt.c_str() + 8, 16);
    }
    if (bpfFilter.size() == 0) {
        //snprintf (filter, 256, "ip and (src host %s and dst host %s)", a_src,a_dst);
        bpfFilter = "ip and (src host ";
        ipaddress helper = dst;
        ipaddress_formatted_t addrText = ipaddress_fmt(dst);
        bpfFilter += addrText.string;
        bpfFilter += " and dst host ";
        addrText = ipaddress_fmt(src);
        bpfFilter += addrText.string;
        bpfFilter += ")";
    }
    HSocket hSocket = raw_open_socket(dst, NULL, bpfFilter.c_str());

    if (hSocket == NULL) {
        return Value();
    }

    if (0 != raw_socket_send(hSocket, (const unsigned char*)pkt.c_str(),(unsigned int)pkt.size())) {
        raw_close_socket(hSocket);
        return Value();
    }
    if (!readAnswer) {
        return Value();
    }
    time_t start = time(NULL);
    while (true) {
        const unsigned char* pkt = NULL;
        unsigned int pkt_size = 0;
        if (time(NULL) - start > timeout) {
            raw_close_socket(hSocket);
            return Value();
        }
        if (0 != raw_socket_recv(hSocket, &pkt, &pkt_size)) {
            continue;
        }
        std::string answer;
        answer.assign((const char*)pkt + 14, pkt_size - 14);
        raw_close_socket(hSocket);
        return answer;
    }
}
//ifname,bpf_filter,time_out
Value CapturePacket(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_INTEGER(2);
    std::string ifname = GetString(args, 0);
    std::string bpfFilter = GetString(args, 1);
    int timeout = GetInt(args, 2, 5);
    CaptureHandle Handle = OpenCapture(ifname.c_str(), bpfFilter.c_str());
    if (Handle == NULL) {
        return Value();
    }
    time_t start = time(NULL);
    while (true) {
        const unsigned char* pkt = NULL;
        unsigned int pkt_size = 0;
        if (time(NULL) - start > timeout) {
            CloseCapture(Handle);
            return Value();
        }
        if (0 != CapturePacket(Handle, &pkt, &pkt_size)) {
            continue;
        }
        std::string answer;
        answer.assign((const char*)pkt + 14, pkt_size - 14);
        CloseCapture(Handle);
        return answer;
    }
}