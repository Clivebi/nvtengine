#include <stdlib.h>

#include <string>

namespace utf8 {
// The default lowest and highest continuation byte.
uint8_t locb = 0b10000000;
uint8_t hicb = 0b10111111;

uint8_t maskx = 0b00111111;
uint8_t mask2 = 0b00011111;
uint8_t mask3 = 0b00001111;
uint8_t mask4 = 0b00000111;

uint8_t t1 = 0b00000000;
uint8_t tx = 0b10000000;
uint8_t t2 = 0b11000000;
uint8_t t3 = 0b11100000;
uint8_t t4 = 0b11110000;
uint8_t t5 = 0b11111000;

int32_t rune1Max = (1 << 7) - 1;
int32_t rune2Max = (1 << 11) - 1;
int32_t rune3Max = (1 << 16) - 1;

// These names of these constants are chosen to give nice alignment in the
// table below. The first nibble is an index into acceptRanges or F for
// special one-byte cases. The second nibble is the Rune length or the
// Status for the special one-byte case.
uint8_t xx = 0xF1; // invalid: size 1
uint8_t as = 0xF0; // ASCII: size 1
uint8_t s1 = 0x02; // accept 0, size 2
uint8_t s2 = 0x13; // accept 1, size 3
uint8_t s3 = 0x03; // accept 0, size 3
uint8_t s4 = 0x23; // accept 2, size 3
uint8_t s5 = 0x34; // accept 3, size 4
uint8_t s6 = 0x04; // accept 0, size 4
uint8_t s7 = 0x44; // accept 4, size 4
// first is information about the first byte in a UTF-8 sequence.
uint8_t first[256] = {
        //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
        //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
        xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
        s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
        s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
        s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
};

// acceptRange gives the range of valid values for the second byte in a UTF-8
// sequence.
//type acceptRange struct {
//	lo uint8 // lowest value for second byte.
//	hi uint8 // highest value for second byte.
//}
//// acceptRanges has size 16 to avoid bounds checks in the code that uses it.
//var acceptRanges = [16]acceptRange{
//	0: {locb, hicb},
//	1: {0xA0, hicb},
//	2: {locb, 0x9F},
//	3: {0x90, hicb},
//	4: {locb, 0x8F},
//}
struct acceptRange {
    uint8_t lo;
    uint8_t hi;
};

acceptRange acceptRanges[16] = {{locb, hicb}, {0xA0, hicb}, {locb, 0x9F}, {0x90, hicb},
                                {locb, 0x8F}, {0, 0},       {0, 0},       {0, 0},
                                {0, 0},       {0, 0},       {0, 0},       {0, 0},
                                {0, 0},       {0, 0},       {0, 0},       {0, 0}};

int32_t RuneError = 0xFFFD; // the "error" Rune or "Unicode replacement character"
        // characters below RuneSelf are represented as themselves in a single byte.
uint8_t RuneSelf =
        0x80; // characters below RuneSelf are represented as themselves in a single byte.
int32_t MaxRune = 0x0010FFFF; // Maximum valid Unicode code point.
int32_t UTFMax = 4;

int32_t DecodeRune(std::string s, int& decode_size) {
    if (s.size() < 1) {
        return RuneError;
    }
    uint8_t s0 = s[0];
    uint8_t x = first[s0];
    if (x >= as) {
        int32_t mask = int32_t(x) << 31 >> 31;
        decode_size = 1;
        return (int32_t(s[0]) & mask) ^ mask | RuneError & mask;
    }
    int32_t sz = x & 7;
    acceptRange accept = acceptRanges[x >> 4];
    if (s.size() <(size_t) sz) {
        decode_size = 1;
        return RuneError;
    }
    uint8_t s1 = s[1];
    if (s1 < accept.lo || accept.hi < s1) {
        decode_size = 1;
        return RuneError;
    }
    if (sz <= 2) {
        decode_size = 2;
        return int32_t(s0 & mask2) << 6 | int32_t(s1 & maskx);
    }
    uint8_t s2 = s[2];
    if (s2 < locb || hicb < s2) {
        decode_size = 1;
        return RuneError;
    }
    if (sz <= 3) {
        decode_size = 3;
        return int32_t(s0 & mask2) << 12 | int32_t(s1 & maskx) << 6 | int32_t(s2 & maskx);
    }
    uint8_t s3 = s[3];
    if (s3 < locb || hicb < s3) {
        decode_size = 1;
        return RuneError;
    }
    decode_size = 4;
    return int32_t(s0 & mask4) << 18 | int32_t(s1 & maskx) << 12 | int32_t(s2 & maskx) << 6 |
           int32_t(s3 & maskx);
}

#define TO_BYTE(x) (uint8_t)(x)

std::string EncodeRune(int32_t r) {
    char buffer[4];
    std::string ret;
    if (r > MaxRune || r < 0) {
        return "";
    }
    uint32_t i = uint32_t(r);
    if (i <= (size_t)rune1Max) {
        buffer[0] = r;
        ret.assign(buffer, 1);
    } else if (i <= (size_t)rune2Max) {
        buffer[0] = t2 | TO_BYTE(r >> 6);
        buffer[1] = tx | TO_BYTE(r) & maskx;
        ret.assign(buffer, 2);
    } else if (i <= (size_t)rune3Max) {
        buffer[0] = t3 | TO_BYTE(r >> 12);
        buffer[1] = tx | TO_BYTE(r >> 6) & maskx;
        buffer[2] = tx | TO_BYTE(r) & maskx;
        ret.assign(buffer, 3);
    } else {
        buffer[0] = t4 | TO_BYTE(r >> 18);
        buffer[1] = tx | TO_BYTE(r >> 12) & maskx;
        buffer[2] = tx | TO_BYTE(r >> 6) & maskx;
        buffer[3] = tx | TO_BYTE(r) & maskx;
        ret.assign(buffer, 4);
    }
    return ret;
}

unsigned int ParseHex4(const unsigned char* const input) {
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++) {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9')) {
            h += (unsigned int)input[i] - '0';
        } else if ((input[i] >= 'A') && (input[i] <= 'F')) {
            h += (unsigned int)10 + input[i] - 'A';
        } else if ((input[i] >= 'a') && (input[i] <= 'f')) {
            h += (unsigned int)10 + input[i] - 'a';
        } else /* invalid */
        {
            return 0;
        }

        if (i < 3) {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

std::string RuneString(int32_t rune) {
    if (rune > MaxRune || rune < 0) {
        return "";
    }
    char buffer[6] = {0};
    snprintf(buffer, 6, "%04x", rune);
    return buffer;
}
} // namespace utf8
