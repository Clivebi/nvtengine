#pragma once
#include <mutex>

#include "engine/vm.hpp"
#include "filepath.hpp"
class BlockFile : public FileIO {
protected:
    struct FileIndex {
        long long Offset;
        unsigned int Size;
        unsigned int Crc32;
        std::string Name;
        FileIndex() {
            Offset = 0;
            Size = 0;
            Crc32 = 0;
            Name = "";
        }
    };
    FilePath mPath;
    FILE* mhFile;
    long long mOffset;
    bool mWriteable;
    std::mutex mLock;
    std::map<std::string, FileIndex*> mFileTable;

    BlockFile(FilePath host)
            : mPath(host), mFileTable(), mhFile(NULL), mOffset(0), mLock(), mWriteable(false) {}

public:
    ~BlockFile() { Close(); }

    bool Close() {
        if (mhFile == NULL) {
            return true;
        }
        if (mWriteable) {
            if (!WriteIndexFile()) {
                return false;
            }
        }
        fclose(mhFile);
        mhFile = NULL;
        for (auto iter : mFileTable) {
            delete (iter.second);
        }
        mFileTable.clear();
        return true;
    }

protected:
    bool WriteIndex(FILE* hFile, FileIndex* index) {
        unsigned short nameSize = 0;
        if (index->Name.size() > 65530) {
            std::__throw_runtime_error("too larger file name");
        }
        if (sizeof(long long) != fwrite(&index->Offset, 1, sizeof(long long), hFile)) {
            return false;
        }
        if (sizeof(unsigned int) != fwrite(&index->Size, 1, sizeof(unsigned int), hFile)) {
            return false;
        }
        if (sizeof(unsigned int) != fwrite(&index->Crc32, 1, sizeof(unsigned int), hFile)) {
            return false;
        }
        nameSize = index->Name.size();
        if (sizeof(unsigned short) != fwrite(&nameSize, 1, sizeof(nameSize), hFile)) {
            return false;
        }
        if ((unsigned int)nameSize != fwrite(index->Name.c_str(), 1, nameSize, hFile)) {
            return false;
        }
        return true;
    }

    bool ReadIndex(FILE* hFile, FileIndex* index) {
        unsigned short nameSize = 0;
        if (sizeof(long long) != fread(&index->Offset, 1, sizeof(long long), hFile)) {
            return feof(hFile);
        }
        if (sizeof(unsigned int) != fread(&index->Size, 1, sizeof(unsigned int), hFile)) {
            return false;
        }
        if (sizeof(unsigned int) != fread(&index->Crc32, 1, sizeof(unsigned int), hFile)) {
            return false;
        }
        if (sizeof(nameSize) != fread(&nameSize, 1, sizeof(nameSize), hFile)) {
            return false;
        }
        char* buffer = new char[(unsigned int)nameSize];
        if ((unsigned int)nameSize != fread(buffer, 1, nameSize, hFile)) {
            delete[] buffer;
            return false;
        }
        index->Name.assign(buffer, nameSize);
        delete[] buffer;
        return true;
    }
    bool WriteIndexFile() {
        FilePath path = std::string(mPath) + ".index";
        FILE* hFile = NULL;
        hFile = fopen(((std::string)path).c_str(), "wb");
        if (hFile == NULL) {
            return false;
        }
        for (auto iter : mFileTable) {
            if (!WriteIndex(hFile, iter.second)) {
                fclose(hFile);
                return false;
            }
        }
        LOG_DEBUG("Total Size:",mOffset/1024/1024,"kb FileCount:",mFileTable.size());
        fclose(hFile);
        return true;
    }
    bool LoadIndexFile() {
        FilePath path = std::string(mPath) + ".index";
        FILE* hFile = NULL;
        hFile = fopen(((std::string)path).c_str(), "rb");
        if (hFile == NULL) {
            return false;
        }
        while (true) {
            FileIndex* index = new FileIndex();
            if (!ReadIndex(hFile, index)) {
                fclose(hFile);
                return false;
            }
            if (index->Size == 0) {
                delete index;
                break;
            }
            mFileTable[index->Name] = index;
        }
        fclose(hFile);
        return true;
    }

public:
    bool IsFileExist(const std::string& name) { return mFileTable.end() != mFileTable.find(name); }
    bool ReadAt(void* buf, long long offset, int size) {
        size_t nRead = 0;
        std::lock_guard guard(mLock);
        do {
            if (0 != fseek(mhFile, offset, SEEK_SET)) {
                break;
            }
            nRead = fread(buf, 1, size, mhFile);
        } while (0);
        return nRead == size;
    }
    void* Read(const std::string& name, size_t& contentSize) {
        auto iter = mFileTable.find(name);
        if (iter == mFileTable.end()) {
            return NULL;
        }
        void* ptr = malloc(iter->second->Size);
        contentSize = iter->second->Size;
        if (ReadAt(ptr, iter->second->Offset, contentSize)) {
            return ptr;
        }
        free(ptr);
        return NULL;
    }
    size_t Write(const std::string& name, const void* content, size_t contentSize) {
        FileIndex* index = new FileIndex();
        index->Offset = mOffset;
        index->Size = contentSize;
        index->Crc32 = 0;
        index->Name = name;
        size_t nWrite = fwrite(content, 1, contentSize, mhFile);
        if (nWrite != contentSize) {
            std::__throw_runtime_error("write block failed");
        }
        mFileTable[index->Name] = index;
        mOffset += contentSize;
        return nWrite;
    }

public:
    static BlockFile* NewReader(FilePath path) {
        BlockFile* block = new BlockFile(path);
        if (!block->LoadIndexFile()) {
            delete block;
            return NULL;
        }
        block->mhFile = fopen(((std::string)path).c_str(), "rb");
        if (block->mhFile == NULL) {
            delete block;
            return NULL;
        }
        return block;
    }
    static BlockFile* NewWriter(FilePath path) {
        BlockFile* block = new BlockFile(path);
        block->mhFile = fopen(((std::string)path).c_str(), "wb");
        if (block->mhFile == NULL) {
            delete block;
            return NULL;
        }
        block->mWriteable = true;
        return block;
    }
};
