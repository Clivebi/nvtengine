#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../api.hpp"
/*
 {"unixtime", nasl_unixtime},
  {"gettimeofday", nasl_gettimeofday},
  {"localtime", nasl_localtime},
  {"mktime", nasl_mktime},
*/

Value UnixTime(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return (int64_t)time(NULL);
}

Value GetTimeOfDay(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    struct timeval t;
    char str[64];

    if (gettimeofday(&t, NULL) < 0) {
        return "";
    }
    sprintf(str, "%u.%06u", (unsigned int)t.tv_sec, (unsigned int)t.tv_usec);
    return str;
}

Value LocalTime(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    time_t t = time(NULL);
    struct tm* ptm;
    if (args.size()) {
        t = args[0].ToInteger();
    }
    ptm = localtime(&t);
    Value ret = Value::make_map();
    ret["sec"] = ptm->tm_sec;
    ret["min"] = ptm->tm_min;
    ret["hour"] = ptm->tm_hour;
    ret["mday"] = ptm->tm_mday;
    ret["mon"] = ptm->tm_mon;
    ret["year"] = ptm->tm_year + 1900;
    ret["wday"] = ptm->tm_wday;
    ret["yday"] = ptm->tm_yday;
    ret["isdst"] = ptm->tm_isdst;
    return ret;
}

Value MakeTime(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    struct tm tm;
    CHECK_PARAMETER_COUNT(7);
    tm.tm_sec = args[0].Integer;
    tm.tm_min = args[1].Integer;
    tm.tm_hour = args[2].Integer;
    tm.tm_mday = args[3].Integer;
    tm.tm_mon = args[4].Integer;
    tm.tm_year = args[5].Integer;
    if (tm.tm_year > 1900) {
        tm.tm_year -= 1900;
    }
    tm.tm_isdst = args[6].Integer;
    return (int64_t)mktime(&tm);
}

#ifndef DIM
#define DIM(v) (sizeof(v) / sizeof((v)[0]))
#define DIMof(type, member) DIM(((type*)0)->member)
#endif

/* The type used to represent the time here is a string with a fixed
   length.  */
#define ISOTIME_SIZE 19
typedef char my_isotime_t[ISOTIME_SIZE];

/* Correction used to map to real Julian days. */
#define JD_DIFF 1721060L

/* Useful helper macros to avoid problems with locales.  */
#define spacep(p) (*(p) == ' ' || *(p) == '\t')
#define digitp(p) (*(p) >= '0' && *(p) <= '9')

/* The atoi macros assume that the buffer has only valid digits. */
#define atoi_1(p) (*(p) - '0')
#define atoi_2(p) ((atoi_1(p) * 10) + atoi_1((p) + 1))
#define atoi_4(p) ((atoi_2(p) * 100) + atoi_2((p) + 2))

/* Convert an Epoch time to an ISO timestamp. */
static void epoch2isotime(my_isotime_t timebuf, time_t atime) {
    if (atime == (time_t)(-1))
        *timebuf = 0;
    else {
        struct tm* tp;

        tp = gmtime(&atime);
        if (snprintf(timebuf, ISOTIME_SIZE, "%04d%02d%02dT%02d%02d%02d", 1900 + tp->tm_year,
                     tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec) < 0) {
            *timebuf = '\0';
            return;
        }
    }
}

/* Return the current time in ISO format. */
static void get_current_isotime(my_isotime_t timebuf) {
    epoch2isotime(timebuf, time(NULL));
}

/* Check that the 15 bytes in ATIME represent a valid ISO timestamp.
   Returns 0 if ATIME has a valid format.  Note that this function
   does not expect a string but a check just the plain 15 bytes of the
   the buffer without looking at the string terminator.  */
static int check_isotime(const my_isotime_t atime) {
    int i;
    const char* s;

    if (!*atime) return 1;

    for (s = atime, i = 0; i < 8; i++, s++)
        if (!digitp(s)) return 1;
    if (*s != 'T') return 1;
    for (s++, i = 9; i < 15; i++, s++)
        if (!digitp(s)) return 1;
    return 0;
}

/* Return true if STRING holds an isotime string.  The expected format is
     yyyymmddThhmmss
   optionally terminated by white space, comma, or a colon.
 */
static int isotime_p(const char* string) {
    const char* s;
    int i;

    if (!*string) return 0;
    for (s = string, i = 0; i < 8; i++, s++)
        if (!digitp(s)) return 0;
    if (*s != 'T') return 0;
    for (s++, i = 9; i < 15; i++, s++)
        if (!digitp(s)) return 0;
    if (!(!*s || (isascii(*s) && isspace(*s)) || *s == ':' || *s == ','))
        return 0; /* Wrong delimiter.  */

    return 1;
}

/* Scan a string and return true if the string represents the human
   readable format of an ISO time.  This format is:
      yyyy-mm-dd[ hh[:mm[:ss]]]
   Scanning stops at the second space or at a comma.  */
static int isotime_human_p(const char* string) {
    const char* s;
    int i;

    if (!*string) return 0;
    for (s = string, i = 0; i < 4; i++, s++)
        if (!digitp(s)) return 0;
    if (*s != '-') return 0;
    s++;
    if (!digitp(s) || !digitp(s + 1) || s[2] != '-') return 0;
    i = atoi_2(s);
    if (i < 1 || i > 12) return 0;
    s += 3;
    if (!digitp(s) || !digitp(s + 1)) return 0;
    i = atoi_2(s);
    if (i < 1 || i > 31) return 0;
    s += 2;
    if (!*s || *s == ',') return 1; /* Okay; only date given.  */
    if (!spacep(s)) return 0;
    s++;
    if (spacep(s)) return 1; /* Okay, second space stops scanning.  */
    if (!digitp(s) || !digitp(s + 1)) return 0;
    i = atoi_2(s);
    if (i < 0 || i > 23) return 0;
    s += 2;
    if (!*s || *s == ',') return 1; /* Okay; only date and hour given.  */
    if (*s != ':') return 0;
    s++;
    if (!digitp(s) || !digitp(s + 1)) return 0;
    i = atoi_2(s);
    if (i < 0 || i > 59) return 0;
    s += 2;
    if (!*s || *s == ',') return 1; /* Okay; only date, hour and minute given.  */
    if (*s != ':') return 0;
    s++;
    if (!digitp(s) || !digitp(s + 1)) return 0;
    i = atoi_2(s);
    if (i < 0 || i > 60) return 0;
    s += 2;
    if (!*s || *s == ',' || spacep(s))
        return 1; /* Okay; date, hour and minute and second given.  */

    return 0; /* Unexpected delimiter.  */
}

/* Convert a standard isotime or a human readable variant into an
   isotime structure.  The allowed formats are those described by
   isotime_p and isotime_human_p.  The function returns 0 on failure
   or the length of the scanned string on success.  */
static int string2isotime(my_isotime_t atime, const char* string) {
    my_isotime_t dummyatime;

    if (!atime) atime = dummyatime;

    memset(atime, '\0', sizeof(my_isotime_t));
    atime[0] = 0;
    if (isotime_p(string)) {
        memcpy(atime, string, 15);
        atime[15] = 0;
        return 15;
    }
    if (!isotime_human_p(string)) return 0;
    atime[0] = string[0];
    atime[1] = string[1];
    atime[2] = string[2];
    atime[3] = string[3];
    atime[4] = string[5];
    atime[5] = string[6];
    atime[6] = string[8];
    atime[7] = string[9];
    atime[8] = 'T';
    if (!spacep(string + 10)) return 10;
    if (spacep(string + 11)) return 11; /* As per def, second space stops scanning.  */
    atime[9] = string[11];
    atime[10] = string[12];
    if (string[13] != ':') return 13;
    atime[11] = string[14];
    atime[12] = string[15];
    if (string[16] != ':') return 16;
    atime[13] = string[17];
    atime[14] = string[18];
    return 19;
}

/* Helper for jd2date.  */
static int days_per_year(int y) {
    int s;

    s = !(y % 4);
    if (!(y % 100))
        if ((y % 400)) s = 0;
    return s ? 366 : 365;
}

/* Helper for jd2date.  */
static int days_per_month(int y, int m) {
    int s;

    switch (m) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 2:
        s = !(y % 4);
        if (!(y % 100))
            if ((y % 400)) s = 0;
        return s ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    default:
        abort();
    }
}

/* Convert YEAR, MONTH and DAY into the Julian date.  We assume that
   it is already noon.  We do not support dates before 1582-10-15. */
static unsigned long date2jd(int year, int month, int day) {
    unsigned long jd;

    jd = 365L * year + 31 * (month - 1) + day + JD_DIFF;
    if (month < 3)
        year--;
    else
        jd -= (4 * month + 23) / 10;

    jd += year / 4 - ((year / 100 + 1) * 3) / 4;

    return jd;
}

/* Convert a Julian date back to YEAR, MONTH and DAY.  Return day of
   the year or 0 on error.  This function uses some more or less
   arbitrary limits, most important is that days before 1582-10-15 are
   not supported. */
static int jd2date(unsigned long jd, int* year, int* month, int* day) {
    int y, m, d;
    long delta;

    if (!jd) return 0;
    if (jd < 1721425 || jd > 2843085) return 0;

    y = (jd - JD_DIFF) / 366;
    d = m = 1;

    while ((delta = jd - date2jd(y, m, d)) > days_per_year(y)) y++;

    m = (delta / 31) + 1;
    while ((delta = jd - date2jd(y, m, d)) > days_per_month(y, m))
        if (++m > 12) {
            m = 1;
            y++;
        }

    d = delta + 1;
    if (d > days_per_month(y, m)) {
        d = 1;
        m++;
    }
    if (m > 12) {
        m = 1;
        y++;
    }

    if (year) *year = y;
    if (month) *month = m;
    if (day) *day = d;

    return (jd - date2jd(y, 1, 1)) + 1;
}

/* Add SECONDS to ATIME.  SECONDS may not be negative and is limited
   to about the equivalent of 62 years which should be more then
   enough for our purposes.  Returns 0 on success.  */
static int add_seconds_to_isotime(my_isotime_t atime, int nseconds) {
    int year, month, day, hour, minute, sec, ndays;
    unsigned long jd;

    if (check_isotime(atime)) return 1;

    if (nseconds < 0 || nseconds >= (0x7fffffff - 61)) return 1;

    year = atoi_4(atime + 0);
    month = atoi_2(atime + 4);
    day = atoi_2(atime + 6);
    hour = atoi_2(atime + 9);
    minute = atoi_2(atime + 11);
    sec = atoi_2(atime + 13);

    /* The julian date functions don't support this. */
    if (year < 1582 || (year == 1582 && month < 10) || (year == 1582 && month == 10 && day < 15))
        return 1;

    sec += nseconds;
    minute += sec / 60;
    sec %= 60;
    hour += minute / 60;
    minute %= 60;
    ndays = hour / 24;
    hour %= 24;

    jd = date2jd(year, month, day) + ndays;
    jd2date(jd, &year, &month, &day);

    if (year > 9999 || month > 12 || day > 31 || year < 0 || month < 1 || day < 1) return 1;

    if (snprintf(atime, ISOTIME_SIZE, "%04d%02d%02dT%02d%02d%02d", year, month, day, hour, minute,
                 sec) < 0)
        return 1;

    return 0;
}

/* Add NDAYS to ATIME.  Returns 0 on success.  */
static int add_days_to_isotime(my_isotime_t atime, int ndays) {
    int year, month, day, hour, minute, sec;
    unsigned long jd;

    if (check_isotime(atime)) return 1;

    if (ndays < 0 || ndays >= 9999 * 366) return 1;

    year = atoi_4(atime + 0);
    month = atoi_2(atime + 4);
    day = atoi_2(atime + 6);
    hour = atoi_2(atime + 9);
    minute = atoi_2(atime + 11);
    sec = atoi_2(atime + 13);

    /* The julian date functions don't support this. */
    if (year < 1582 || (year == 1582 && month < 10) || (year == 1582 && month == 10 && day < 15))
        return 1;

    jd = date2jd(year, month, day) + ndays;
    jd2date(jd, &year, &month, &day);

    if (year > 9999 || month > 12 || day > 31 || year < 0 || month < 1 || day < 1) return 1;

    if (snprintf(atime, ISOTIME_SIZE, "%04d%02d%02dT%02d%02d%02d", year, month, day, hour, minute,
                 sec) < 0)
        return 1;
    return 0;
}

/* Add NYEARS to ATIME.  Returns 0 on success.  */
static int add_years_to_isotime(my_isotime_t atime, int nyears) {
    int year, month, day, hour, minute, sec;
    unsigned long jd;

    if (check_isotime(atime)) return 1;

    if (nyears < 0 || nyears >= 9999) return 1;

    year = atoi_4(atime + 0);
    month = atoi_2(atime + 4);
    day = atoi_2(atime + 6);
    hour = atoi_2(atime + 9);
    minute = atoi_2(atime + 11);
    sec = atoi_2(atime + 13);

    /* The julian date functions don't support this. */
    if (year < 1582 || (year == 1582 && month < 10) || (year == 1582 && month == 10 && day < 15))
        return 1;

    jd = date2jd(year + nyears, month, day);
    jd2date(jd, &year, &month, &day);

    if (year > 9999 || month > 12 || day > 31 || year < 0 || month < 1 || day < 1) return 1;

    if (snprintf(atime, ISOTIME_SIZE, "%04d%02d%02dT%02d%02d%02d", year, month, day, hour, minute,
                 sec) < 0)
        return 1;

    return 0;
}

Value ISOTimeNow(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    my_isotime_t timebuf;
    get_current_isotime(timebuf);
    return timebuf;
}

Value ISOTimeIsValid(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    if (args[0].Length() < ISOTIME_SIZE - 1) {
        return Value(false);
    }
    my_isotime_t timebuf = {0};
    memcpy(timebuf, args[0].text.c_str(), args[0].text.size());
    if (isotime_p(timebuf) || isotime_human_p(timebuf)) {
        return Value(true);
    }
    return Value(false);
}

Value ISOTimeScan(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    my_isotime_t timebuf = {0};
    if (args[0].Length() < ISOTIME_SIZE - 1) {
        return Value();
    }
    memcpy(timebuf, args[0].text.c_str(), ISOTIME_SIZE - 1);
    timebuf[ISOTIME_SIZE - 1] = 0;
    if (!string2isotime(timebuf, timebuf)) {
        return Value();
    }
    return Value(timebuf);
}

Value ISOTimePrint(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    const char* string = args[0].text.c_str();
    char helpbuf[20];
    if (args[0].Length() < 15 || check_isotime(string)) {
        strcpy(helpbuf, "[none]");
    } else {
        //2021-06-01 01:16:03
        snprintf(helpbuf, sizeof helpbuf, "%.4s-%.2s-%.2s %.2s:%.2s:%.2s", string, string + 4,
                 string + 6, string + 9, string + 11, string + 13);
    }
    return helpbuf;
}

Value ISOTimeAdd(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_INTEGER(1);
    CHECK_PARAMETER_INTEGER(2);
    CHECK_PARAMETER_INTEGER(3);

    my_isotime_t timebuf;
    const char* string = args[0].text.c_str();
    int nyears = GetInt(args, 1, 0), ndays = GetInt(args, 2, 0), nseconds = GetInt(args, 3, 0);
    memcpy(timebuf, string, ISOTIME_SIZE - 1);
    timebuf[ISOTIME_SIZE - 1] = 0;

    if (nyears && add_years_to_isotime(timebuf, nyears)) return Value();
    if (ndays && add_days_to_isotime(timebuf, ndays)) return Value();
    if (nseconds && add_seconds_to_isotime(timebuf, nseconds)) return Value();
    /* If nothing was added, explicitly add 0 years.  */
    if (!nyears && !ndays && !nseconds && add_years_to_isotime(timebuf, 0)) return Value();
    return timebuf;
}
