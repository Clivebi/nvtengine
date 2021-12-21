#pragma once
#include <string>
class FilePath {
protected:
    std::string _full;
#ifdef _WIN32
    static const char s_separator = '\\';
#else
    static const char s_separator = '/';
#endif
public:
    FilePath(const char* src) : _full(src) {}
    FilePath(std::string src) : _full(src) {}
    FilePath(const FilePath& src) : _full(src._full) {}

    FilePath& operator+=(const FilePath& part) {
        if (part._full.size() == 0) {
            return *this;
        }
        if (_full.size() && _full.back() != s_separator && part._full[0] != s_separator) {
            _full += s_separator;
        }
        _full += part._full;
        return *this;
    }
    FilePath operator+(const FilePath& right) {
        FilePath ret(*this);
        ret += right;
        return ret;
    }

    FilePath& operator+=(const char* part) {
        std::string right(part);
        return this->operator+=(right);
    }

    operator std::string() { return _full; }
    std::string base_name() {
        std::string part = _full;
        if (part.size() && part.back() == s_separator) {
            part = _full.substr(0, part.size() - 1);
        }
        size_t i = part.rfind(s_separator);
        if (i == part.npos) {
            return part;
        }
        return part.substr(i);
    }

    std::string extension_name() {
        size_t i = _full.rfind('.');
        if (i == _full.npos || i >= _full.size()) {
            return "";
        }
        return _full.substr(i + 1);
    }
};
