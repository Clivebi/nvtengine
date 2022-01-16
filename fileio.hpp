#pragma once
#include <fstream>

#include "engine/logger.hpp"
#include "filepath.hpp"

class FileIO {
public:
    virtual bool Write(const std::string& path, const std::string& data) = 0;
    virtual void* Read(const std::string& path, size_t& nSize) = 0;
};

class StdFileIO : public FileIO {
protected:
    FilePath mDirectory;

public:
    explicit StdFileIO(FilePath dir) : mDirectory(dir) {}
    std::string ResolvePath(const std::string& path) {
#ifdef _WIN32
        if (path.size() > 3 && path[1] == ':' && path[2] == "\\") {
            return path;
        }
#else
        if (path[0] == '/') {
            return path;
        }
#endif
        return mDirectory + FilePath(path);
    }

public:
    bool Write(const std::string& path, const std::string& data) {
        FILE* hFile = NULL;
        hFile = fopen(ResolvePath(path).c_str(), "wb");
        if (hFile == NULL) {
            return false;
        }
        size_t nWrite = fwrite(data.c_str(), 1, data.size(), hFile);
        fclose(hFile);
        return nWrite == data.size();
    }
    void* Read(const std::string& path, size_t& nSize) {
        FILE* hFile = NULL;
        void* pData = NULL;
        size_t size = 0, read_size = 0;
        hFile = fopen(ResolvePath(path).c_str(), "rb");
        if (hFile == NULL) {
            LOG_ERROR("fopen failed: ", errno, " ",ResolvePath(path));
            return NULL;
        }
        fseek(hFile, 0, SEEK_END);
        nSize = ftell(hFile);
        fseek(hFile, 0, SEEK_SET);
        pData = malloc(nSize);
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
        return pData;
    }
};
