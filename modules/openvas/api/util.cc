#include <regex.h> /* for regex_t */
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "../api.hpp"

Value match(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    bool icase = false;
    if (args.size() > 2 && args[2].ToBoolean()) {
        icase = true;
    }
    if (icase) {
        std::string src = args[0].bytes;
        std::string pattern = args[1].bytes;
        std::transform(src.begin(), src.end(), src.begin(), tolower);
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), tolower);
        return Interpreter::IsMatchString(src, pattern);
    }
    return Interpreter::IsMatchString(args[0].bytes, args[1].bytes);
}

Value Rand(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    srand((int)time(NULL));
    return rand();
}

Value USleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef WIN32
    Sleep(args[0].Integer);
#else
    usleep(args[0].Integer);
#endif
    return Value();
}

Value Sleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef WIN32
    Sleep(args[0].Integer * 1000);
#else
    sleep(args[0].Integer);
#endif
    return Value();
}

Value vendor_version(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return Value("NVTEngine 0.1");
}

Value GetHostName(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    char name[260] = {0};
    unsigned int size = 260;
    gethostname(name, size);
    return Value(name);
}

bool ishex(char c);

char http_unhex(char c);

std::string decode_string(const std::string& src) {
    std::stringstream o;
    size_t start = 0;
    for (size_t i = 0; i < src.size();) {
        if (src[i] != '\\') {
            i++;
            continue;
        }
        if (start < i) {
            o << src.substr(start, i - start);
        }
        if (i + 1 >= src.size()) {
            return src;
        }
        i++;
        switch (src[i]) {
        case 'r':
            o << "\r";
            i++;
            break;
        case 'n':
            o << "\n";
            i++;
            break;
        case 't':
            o << "\t";
            i++;
            break;
        case '\"':
            o << "\"";
            i++;
            break;
        case '\\':
            o << "\\";
            i++;
            break;
        default: {
            if (src.size() > i + 1 && ishex(src[i]) && ishex(src[i + 1])) {
                int x = http_unhex(src[i]);
                x *= 16;
                x += http_unhex(src[i + 1]);
                o << (char)(x & 0xFF);
                i += 2;
            } else {
                i++;
                o << src[i];
                LOG("Parse string error :" + src);
            }
        }
        }
        start = i;
    }
    if (start < src.size()) {
        o << src.substr(start);
    }
    return o.str();
}

Value NASLString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    std::string ret = "";
    for (auto iter : args) {
        ret += decode_string(iter.ToString());
    }
    return ret;
}

static char* _regreplace(const char* pattern, const char* replace, const char* string, int icase,
                         int extended) {
    regex_t re;
    regmatch_t subs[60];

    char *buf,        /* buf is where we build the replaced string */
            *nbuf,    /* nbuf is used when we grow the buffer */
            *walkbuf; /* used to walk buf when replacing backrefs */
    const char* walk; /* used to walk replacement string for backrefs */
    int buf_len;
    int pos, tmp, string_len, new_l;
    int err, copts = 0;

    string_len = strlen(string);

    if (icase) copts = REG_ICASE;
    if (extended) copts |= REG_EXTENDED;
    err = regcomp(&re, pattern, copts);
    if (err) {
        return NULL;
    }

    /* start with a buffer that is twice the size of the stringo
     we're doing replacements in */
    buf_len = 2 * string_len;
    buf = (char*)malloc(buf_len + 1);

    err = pos = 0;
    buf[0] = '\0';

    while (!err) {
        err = regexec(&re, &string[pos], (size_t)6, subs, (pos ? REG_NOTBOL : 0));

        if (err && err != REG_NOMATCH) {
            free(buf);
            return (NULL);
        }
        if (!err) {
            /* backref replacement is done in two passes:
             1) find out how long the string will be, and allocate buf
             2) copy the part before match, replacement and backrefs to buf

             Jaakko Hyv�tti <Jaakko.Hyvatti@iki.fi>
           */

            new_l = strlen(buf) + subs[0].rm_so; /* part before the match */
            walk = replace;
            while (*walk)
                if ('\\' == *walk && '0' <= walk[1] && '9' >= walk[1] &&
                    subs[walk[1] - '0'].rm_so > -1 && subs[walk[1] - '0'].rm_eo > -1) {
                    new_l += subs[walk[1] - '0'].rm_eo - subs[walk[1] - '0'].rm_so;
                    walk += 2;
                } else {
                    new_l++;
                    walk++;
                }

            if (new_l + 1 > buf_len) {
                buf_len = buf_len + 2 * new_l;
                nbuf = (char*)malloc(buf_len + 1);
                strncpy(nbuf, buf, buf_len);
                free(buf);
                buf = nbuf;
            }
            tmp = strlen(buf);
            /* copy the part of the string before the match */
            strncat(buf, &string[pos], subs[0].rm_so);

            /* copy replacement and backrefs */
            walkbuf = &buf[tmp + subs[0].rm_so];
            walk = replace;
            while (*walk)
                if ('\\' == *walk && '0' <= walk[1] && '9' >= walk[1] &&
                    subs[walk[1] - '0'].rm_so > -1 && subs[walk[1] - '0'].rm_eo > -1) {
                    tmp = subs[walk[1] - '0'].rm_eo - subs[walk[1] - '0'].rm_so;
                    memcpy(walkbuf, &string[pos + subs[walk[1] - '0'].rm_so], tmp);
                    walkbuf += tmp;
                    walk += 2;
                } else
                    *walkbuf++ = *walk++;
            *walkbuf = '\0';

            /* and get ready to keep looking for replacements */
            if (subs[0].rm_so == subs[0].rm_eo) {
                if (subs[0].rm_so + pos >= string_len) break;
                new_l = strlen(buf) + 1;
                if (new_l + 1 > buf_len) {
                    buf_len = buf_len + 2 * new_l;
                    nbuf = (char*)malloc(buf_len + 1);
                    strncpy(nbuf, buf, buf_len);
                    free(buf);
                    buf = nbuf;
                }
                pos += subs[0].rm_eo + 1;
                buf[new_l - 1] = string[pos - 1];
                buf[new_l] = '\0';
            } else {
                pos += subs[0].rm_eo;
            }
        } else { /* REG_NOMATCH */
            new_l = strlen(buf) + strlen(&string[pos]);
            if (new_l + 1 > buf_len) {
                buf_len = new_l; /* now we know exactly how long it is */
                nbuf = (char*)malloc(buf_len + 1);
                strncpy(nbuf, buf, buf_len);
                free(buf);
                buf = nbuf;
            }
            /* stick that last bit of string on our output */
            strcat(buf, &string[pos]);
        }
    }

    buf[new_l] = '\0';
    regfree(&re);
    /* whew. */
    return (buf);
}
Value ereg_replace(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    int icase = GetInt(args, 3, 0);
    char* buf = _regreplace(args[1].bytes.c_str(), args[2].bytes.c_str(), args[0].bytes.c_str(),
                            icase, 1);
    if (buf == NULL) {
        return Value();
    }
    std::string ret = buf;
    free(buf);
    return ret;
}