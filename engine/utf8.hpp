#include <stdlib.h>

#include <string>

namespace utf8 {
extern int32_t RuneError;
extern uint8_t RuneSelf;
extern int32_t MaxRune;
extern int32_t UTFMax;

int32_t DecodeRune(std::string s, int& decode_size);

unsigned int ParseHex4(const unsigned char* const input);

std::string EncodeRune(int32_t r);

std::string RuneString(int32_t rune);
} // namespace utf8
