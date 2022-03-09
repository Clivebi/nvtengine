#pragma once
#include <string.h>

#include "../engine/value.hpp" //
class Reader {
public:
    virtual ~Reader() {}
    virtual int Read(void* buffer, int size) = 0;
};

class Writer {
public:
    virtual ~Writer() {}
    virtual int Write(const void* buffer, int size) = 0;
};

class ReadWriter : public Reader, public Writer {};

class BufferedReader : Reader {
protected:
    int mPos;
    int mCapSize;
    int mSize;
    unsigned char* mBuffer;
    Reader* mReader;

    DISALLOW_COPY_AND_ASSIGN(BufferedReader);

public:
    explicit BufferedReader(Reader* reader, const void* init = NULL, int initSize = 0,
                            int cap = 16 * 1024) {
        #ifdef _WIN32
        cap = max(cap, initSize);
        #else
        cap = std::max(cap, initSize);
        #endif
        mBuffer = new unsigned char[cap];
        mCapSize = cap;
        mPos = 0;
        mSize = 0;
        mReader = reader;
        if (init && initSize > 0) {
            memcpy(mBuffer, init, initSize);
            mSize = initSize;
        }
    }
    ~BufferedReader() { delete[] mBuffer; }

    int Read(void* buffer, int size) {
        if (mPos < mSize) {
#ifdef _WIN32
            size =min(size, (mSize - mPos));
#else
            size = std::min(size, (mSize - mPos));
#endif
            memcpy(buffer, mBuffer + mPos, size);
            mPos += size;
            return size;
        }
        if (mReader == NULL) {
            return EOF;
        }
        int ReadSize = mReader->Read(mBuffer, mCapSize);
        if (ReadSize <= 0) {
            return ReadSize;
        }
        mSize = ReadSize;
        mPos = 0;
#ifdef _WIN32
        size = min(size, (mSize - mPos));
#else
        size = std::min(size, (mSize - mPos));
#endif
        memcpy(buffer, mBuffer + mPos, size);
        mPos += size;
        return size;
    }
};

inline int ReadAtleast(Reader* r, BYTE* buffer, int bufferSize, int minSize) {
    int nPos = 0, i;
    while (true) {
        i = r->Read(buffer + nPos, bufferSize - nPos);
        if (i <= 0) {
            return nPos;
        }
        nPos += i;
        if (minSize > 0 && nPos >= minSize) {
            break;
        }
    }
    return nPos;
}

class MemWriter : public Writer {
public:
    std::string str;
    MemWriter() : str("") {}
    int Write(const void* buffer, int size) {
        str.append((const char*)buffer, size);
        return size;
    }
};