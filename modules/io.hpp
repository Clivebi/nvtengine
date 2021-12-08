#pragma once
#include <string.h>

#include "../engine/value.hpp" //
class Reader {
public:
    virtual int Read(void* buffer, int size) = 0;
};

class Writer {
public:
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

public:
    explicit BufferedReader(Reader* reader, int size = 16 * 1024) {
        mBuffer = new unsigned char[size];
        mCapSize = size;
        mPos = 0;
        mSize = 0;
        mReader = reader;
    }
    ~BufferedReader() { delete[] mBuffer; }

    int Read(void* buffer, int size) {
        if (mPos < mSize) {
            if (size > mSize - mPos) {
                size = mSize - mPos;
            }
            memcpy(buffer, mBuffer + mPos, size);
            mPos += size;
            return size;
        }
        int ReadSize = mReader->Read(mBuffer, mCapSize);
        if (ReadSize <= 0) {
            return ReadSize;
        }
        mSize = ReadSize;
        mPos = 0;
        if (size > mSize - mPos) {
            size = mSize - mPos;
        }
        memcpy(buffer, mBuffer + mPos, size);
        mPos += size;
        return size;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(BufferedReader);
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