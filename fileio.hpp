#pragma once
#include <fstream>

#include "filepath.hpp"
#include "engine/logger.hpp"
class FileIO {
public:
    virtual bool Write(const std::string& path, const std::string& data) {
        FILE* hFile = NULL;
        hFile = fopen(path.c_str(), "w");
        if (hFile == NULL) {
            return false;
        }
        size_t nWrite = fwrite(data.c_str(), 1, data.size(), hFile);
        fclose(hFile);
        return nWrite == data.size();
    }

    virtual void* Read(const std::string& path, size_t& nSize, int SuffixSize = 0) {
        FILE* hFile = NULL;
        void* pData = NULL;
        size_t size = 0, read_size = 0;
        hFile = fopen(path.c_str(), "r");
        if (hFile == NULL) {
            return NULL;
        }
        fseek(hFile, 0, SEEK_END);
        nSize = ftell(hFile);
        fseek(hFile, 0, SEEK_SET);
        pData = malloc(nSize + SuffixSize);
        if (pData == NULL) {
            fclose(hFile);
            return NULL;
        }
        if (nSize != fread(pData, 1, nSize, hFile)) {
            fclose(hFile);
            free(pData);
            return NULL;
        }
        fclose(hFile);
        for (int i = 0; i < SuffixSize; i++) {
            ((char*)pData)[i + nSize] = 0;
        }
        return pData;
    }
};