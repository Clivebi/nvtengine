#pragma once
#include <sstream>
namespace varint {
inline int encode(long long num, unsigned char buf[9]) {
    for (int i = 0; i < 9; i++) {
        buf[i] = (unsigned char)(num & 0x7F);
        num >>= 7;
        if (num == 0) {
            return i+1;
        }
        buf[i] |= (1 << 7);
    }
    return -1;
}

inline int decode(std::istream& stream, long long& result) {
    unsigned char posV = 0;
    result = 0;
    for (int i = 0; i < 9; i++) {
        stream.read((char*)&posV, 1);
        result |= (((long long)posV & 0x7F) << (i * 7));
        if (posV & (1 << 7)) {
            continue;
        }
        return i+1;
    }
    return -1;
}

inline int decode(unsigned char buf[9], long long& result) {
    unsigned char posV = 0;
    result = 0;
    for (int i = 0; i < 9; i++) {
        posV = buf[i];
        result |= (((long long)posV & 0x7F) << (i * 7LL));
        if (posV & (1 << 7)) {
            continue;
        }
        return i+1;
    }
    return -1;
}
} // namespace varint