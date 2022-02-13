#pragma once
#include <fstream>

#include "engine/file.hpp"
#include "engine/logger.hpp"
#include "filepath.hpp"

class StdFileIO : public FileIO {
protected:
    FilePath mDirectory;

public:
    explicit StdFileIO(FilePath dir) : mDirectory(dir) {}
    std::string ResolvePath(const std::string& path) {
#ifdef _WIN32
        if (path.size() > 3 && path[1] == ':' && path[2] == '\\') {
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
    size_t Write(const std::string& name, const void* content, size_t contentSize) {
        FILE* hFile = NULL;
        hFile = fopen(ResolvePath(name).c_str(), "wb");
        if (hFile == NULL) {
            return false;
        }
        size_t nWrite = fwrite(content, 1, contentSize, hFile);
        fclose(hFile);
        return nWrite;
    }
    void* Read(const std::string& name, size_t& contentSize) {
        FILE* hFile = NULL;
        void* pData = NULL;
        size_t size = 0, read_size = 0;
        hFile = fopen(ResolvePath(name).c_str(), "rb");
        if (hFile == NULL) {
            NVT_LOG_ERROR("fopen failed: ", errno, " ", ResolvePath(name));
            return NULL;
        }
        fseek(hFile, 0, SEEK_END);
        contentSize = ftell(hFile);
        fseek(hFile, 0, SEEK_SET);
        pData = malloc(contentSize);
        if (pData == NULL) {
            fclose(hFile);
            NVT_LOG_ERROR("Allocate Memory error...");
            return NULL;
        }
        if (contentSize != fread(pData, 1, contentSize, hFile)) {
            fclose(hFile);
            free(pData);
            NVT_LOG_ERROR("fread error...", errno);
            return NULL;
        }
        fclose(hFile);
        return pData;
    }
};
