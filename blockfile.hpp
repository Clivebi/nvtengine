#pragma once
#include <mutex>

#include "engine/script.hpp"
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
        bool IsZero() { return Offset == 0 && Size == 0 && Name.size() == 0; }
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
    void EncodeIndex(std::ostream& o, FileIndex* index) {
        BinaryWrite(o, index->Offset);
        BinaryWrite(o, index->Size);
        BinaryWrite(o, index->Crc32);
        BinaryWrite(o, index->Name);
    }

    void DecodeIndex(std::istream& i, FileIndex* index) {
        BinaryRead(i, index->Offset);
        BinaryRead(i, index->Size);
        BinaryRead(i, index->Crc32);
        BinaryRead(i, index->Name);
    }
    bool WriteIndexFile() {
        FilePath path = std::string(mPath) + ".index";
        std::ofstream out(std::string(mPath) + ".index",std::ios::binary);
        for (auto iter : mFileTable) {
            EncodeIndex(out, iter.second);
        }
        FileIndex empty;
        EncodeIndex(out, &empty);
        out.close();
        LOG_DEBUG("Total Size : ", mOffset / 1024, " kb File Count:", mFileTable.size());
        return true;
    }
    bool LoadIndexFile() {
        FilePath path = std::string(mPath) + ".index";
        std::ifstream input(std::string(mPath) + ".index", std::ios::binary);
        while (true) {
            FileIndex* index = new FileIndex();
            DecodeIndex(input, index);
            if (index->IsZero()) {
                delete index;
                break;
            }
            mFileTable[index->Name] = index;
        }
        input.close();
        return mFileTable.size() > 0;
    }

public:
    bool IsFileExist(const std::string& name) { return mFileTable.end() != mFileTable.find(name); }
    bool ReadAt(void* buf, long long offset, int size) {
        size_t nRead = 0;
        mLock.lock();
        do {
            if (0 != fseek(mhFile, (long)offset, SEEK_SET)) {
                break;
            }
            nRead = fread(buf, 1, size, mhFile);
        } while (0);
        mLock.unlock();
        return nRead == size;
    }
    void* Read(const std::string& name, size_t& contentSize) {
        auto iter = mFileTable.find(name);
        if (iter == mFileTable.end()) {
            return NULL;
        }
        void* ptr = malloc(iter->second->Size);
        contentSize = iter->second->Size;
        if (ReadAt(ptr, iter->second->Offset, (int)contentSize)) {
            return ptr;
        }
        free(ptr);
        return NULL;
    }
    size_t Write(const std::string& name, const void* content, size_t contentSize) {
        FileIndex* index = new FileIndex();
        index->Offset = mOffset;
        index->Size = (unsigned int)contentSize;
        index->Crc32 = 0;
        index->Name = name;
        size_t nWrite = fwrite(content, 1, contentSize, mhFile);
        if (nWrite != contentSize) {
            throw Interpreter::RuntimeException("write block failed");
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
