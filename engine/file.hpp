#pragma once
#include <string>
class FileReader {
public:
    virtual void* Read(const std::string& name, size_t& contentSize) = 0;
};

class FileWriter {
public:
    virtual size_t Write(const std::string& name, const void* content, size_t contentSize) = 0;
};

class FileIO : public FileReader, public FileWriter {};