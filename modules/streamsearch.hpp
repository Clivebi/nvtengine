
#pragma once
#include <string>
struct StreamSearch {
    std::string keyword;
    int pos;
    int skiped;
    StreamSearch(const char* key) : keyword(key), pos(0),skiped(0) {}
    int process(const char* buf, int size) {
        for (int i = 0; i < size; i++,skiped++) {
            if (keyword[pos] != buf[i]) {
                pos = 0;
                continue;
            }
            pos++;
            if (pos == keyword.size()) {
                return skiped;
            }
        }
        return 0;
    }
};