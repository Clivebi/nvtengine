#pragma once
#include <list>
#include <mutex>

#include "engine/exception.hpp"
#include "engine/file.hpp"
#include "filepath.hpp"
extern "C" {
#include "vfs/vfsreader.h"
}

class VFSFileIO : public FileIO {
protected:
    vfs_handle_p mFile;
    FilePath mWriteDir;

public:
    explicit VFSFileIO(const std::string& path) : mFile(NULL), mWriteDir("") {
        mFile = vfs_open(path.c_str());
        if (mFile == NULL) {
            throw Interpreter::RuntimeException("vfs_open failed:" + path);
        }
        mWriteDir = FilePath(path).dir();
    }
    ~VFSFileIO() {
        if (mFile) {
            vfs_close(mFile);
            mFile = NULL;
        }
    }

    void EnumFile(std::list<std::string>& files) {
        int count = 0;
        const vfs_dir_entry_p* entry_list = vfs_get_all_files(mFile, &count);
        for (int i = 0; i < count; i++) {
            char path[512] = {0};
            int path_size = 512;
            vfs_get_file_full_path(mFile, entry_list[i], path, &path_size);
            path[path_size] = 0;
            files.push_back(path);
        }
    }

    size_t Write(const std::string& name, const void* content, size_t contentSize) {
        StdFileIO w(mWriteDir);
        return w.Write(name, content, contentSize);
    }

    void* Read(const std::string& name, size_t& contentSize) {
        int error = 0;
        vfs_dir_entry_p pEntry = vfs_lookup(mFile, name.c_str());
        if (pEntry == NULL) {
            NVT_LOG_ERROR("file not found:", name);
            return NULL;
        }
        void* pContext = vfs_get_file_all_content(mFile, pEntry, &error);
        if (error != 0) {
            NVT_LOG_ERROR("vfs error ", error);
            return NULL;
        }
        contentSize = vfs_file_size(pEntry);
        return pContext;
    }
};