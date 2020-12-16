/*
 * ti/datetime.c
 */
#define _GNU_SOURCE
#include <ti/datetime.h>
#include <ti/val.t.h>
#include <ti/raw.inline.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define DATETIME__BUF_SZ 80
static char datetime__buf[DATETIME__BUF_SZ];
static const char * datetime__fmt_zone = "%Y-%m-%dT%H:%M:%S%z";
static const char * datetime__fmp_utc = "%Y-%m-%dT%H:%M:%SZ";

static inline const char * datetime__fmt(ti_datetime_t * dt)
{
    return ti_tz_is_not_utc(dt->tz) || dt->offset
            ? datetime__fmt_zone
            : datetime__fmp_utc;
}

/*
 * Arguments `ts` is in seconds, `offset` in minutes.
 */
ti_datetime_t * ti_timeval_from_i64(int64_t ts, int16_t offset, ti_tz_t * tz)
{
    ti_datetime_t * dt = malloc(sizeof(ti_datetime_t));
    if (!dt)
        return NULL;
    dt->ref = 1;
    dt->flags = DT_AS_TIMEVAL;
    dt->tp = TI_VAL_DATETIME;
    dt->ts = (time_t) ts;
    dt->offset = offset;
    dt->tz = tz;  /* may be NULL */
    return dt;
}

/*
 * Arguments `ts` is in seconds, `offset` in minutes.
 */
ti_datetime_t * ti_datetime_from_i64(int64_t ts, int16_t offset, ti_tz_t * tz)
{
    ti_datetime_t * dt = malloc(sizeof(ti_datetime_t));
    if (!dt)
        return NULL;
    dt->ref = 1;
    dt->flags = 0;
    dt->tp = TI_VAL_DATETIME;
    dt->ts = (time_t) ts;
    dt->offset = offset;
    dt->tz = tz;  /* may be NULL */
    return dt;
}

ti_datetime_t * ti_datetime_copy(ti_datetime_t * dt)
{
    ti_datetime_t * dtnew = malloc(sizeof(ti_datetime_t));
    memcpy(dtnew, dt, sizeof(ti_datetime_t));
    dtnew->ref = 1;
    return dtnew;
}

int ti_datetime_time(ti_datetime_t * dt, struct tm * tm)
{
    if (dt->tz)
    {
        ti_tz_set(dt->tz);
        tzset();
        return -(localtime_r(&dt->ts, tm) != tm);
    }

    if (dt->offset)
    {
        long int offset = ((long int) dt->offset) * 60;
        time_t ts = dt->ts;

        if ((offset > 0 && ts > LLONG_MAX - offset) ||
            (offset < 0 && ts < LLONG_MIN - offset))
            return -1;

        ts += offset;

        if (gmtime_r(&ts, tm) != tm)
            return -1;

        tm->tm_gmtoff = offset;
        return 0;
    }

    return -(gmtime_r(&dt->ts, tm) != tm);
}

static long int datetime__offset_in_sec(const char * str, ex_t * e)
{
    int sign;
    long int hours, minutes;

    switch (*str)
    {
    case '-':
        sign = -1;
        break;
    case '+':
        sign = 1;
        break;
    default:
        goto invalid;
    }

    if (!isdigit(*(++str)))
        goto invalid;

    hours = (*str - '0') * 10;

    if (!isdigit(*(++str)))
        goto invalid;

    hours += (*str - '0');

    if (hours > 12)
        goto invalid;

    if (isdigit(*(++str)))
    {
        /* read minutes */
        minutes = (*str - '0') * 10;

        if (!isdigit(*(++str)))
            goto invalid;

        minutes += (*str - '0');

        if (minutes > 59 || (minutes && hours == 12))
            goto invalid;

        ++str;
    }
    else
        minutes = 0;

    if (*str)  /* must be the null terminator character */
        goto invalid;

    return (hours * 3600 + minutes * 60) * sign;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid offset; "
            "expecting format `+/-hh[mm]`, for example +0100 or -05");
    return 0;
}

static ti_datetime_t * datetime__from_strn(
        ti_raw_t * str,
        const char * fmt,
        size_t n,
        ti_tz_t * tz,
        ex_t * e)
{
    ti_datetime_t * dt;
    time_t ts;
    long int offset;
    struct tm tm;
    const size_t buf_sz = 120;
    char buf[buf_sz];
    char * c;

    if (str->n >= buf_sz)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid date/time string (too large)");
        return NULL;
    }

    memcpy(buf, str->data, str->n);
    buf[str->n] = '\0';

    memset(&tm, 0, sizeof(struct tm));
    tm.tm_mday = 1;  /* must be set to 1 */

    c = strptime(buf, fmt, &tm);
    if (c == NULL || *c)
        goto invalid;

    switch (fmt[n-1])
    {
    case 'Z':
        ti_tz_set_utc();
        ts = mktime(&tm);
        offset = 0;
        tz = ti_tz_utc();
        break;
    case 'z':
        {
            ti_tz_set_utc();
            ts = mktime(&tm);
            n = str->n;
            while (n--)
                if (buf[n] == '+' || buf[n] == '-')
                    break;

            if (n > str->n)
                goto invalid;

            offset = datetime__offset_in_sec(buf + n, e);
            if (e->nr)
                return NULL;

            ts -= offset;
            offset /= 60;
            tz = NULL;  /* no time zone info */
        }
        break;
    default:
        ti_tz_set(tz);
        ts = mktime(&tm);
        offset = tm.tm_gmtoff / 60;
    }

    /* offset is checked */
    dt = ti_datetime_from_i64(ts, (int16_t) offset, tz);
    if (!dt)
    {
        ex_set_mem(e);
        return NULL;
    }

    return dt;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid date/time string (does not match format string `%s`)", fmt);
    return NULL;
}

static inline _Bool datetime__with_t(ti_raw_t * str)
{
    return str->data[10] == 'T';
}

#define datetime__from_str(__s, __f, __tz, __e) \
    (datetime__from_strn(__s, __f, strlen(__f), __tz, __e))

ti_datetime_t * ti_datetime_from_str(ti_raw_t * str, ti_tz_t * tz, ex_t * e)
{
    switch(str->n)
    {
    case 4:  /* YYYY */
        return datetime__from_str(str, "%Y", tz, e);
    case 7:  /* YYYY-MM */
        return datetime__from_str(str, "%Y-%m", tz, e);
    case 10: /* YYYY-MM-DD */
        return datetime__from_str(str, "%Y-%m-%d", tz, e);
    case 13: /* YYYY-MM-DDTHH */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H", tz, e)
                : datetime__from_str(str, "%Y-%m-%d %H", tz, e);
    case 16: /* YYYY-MM-DDTHH:MM */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M", tz, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M", tz, e);
    case 19: /* YYYY-MM-DDTHH:MM:SS */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S", tz, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S", tz, e);
    case 20: /* YYYY-MM-DDTHH:MM:SSZ */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%SZ", tz, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%SZ", tz, e);
    case 22: /* YYYY-MM-DDTHH:MM:SS+hh */
    case 24: /* YYYY-MM-DDTHH:MM:SS+hhmm */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S%z", tz, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S%z", tz, e);
    }

    ex_set(e, EX_VALUE_ERROR,
            "invalid date/time string");
    return NULL;
}

ti_datetime_t * ti_datetime_from_fmt(
        ti_raw_t * str,
        ti_raw_t * fmt,
        ti_tz_t * tz,
        ex_t * e)
{
    if (fmt->n < 2 || fmt->n >= DATETIME__BUF_SZ)
    {
        ex_set(e, EX_VALUE_ERROR, "invalid date/time format (wrong size)");
        return NULL;
    }

    memcpy(datetime__buf, fmt->data, fmt->n);
    datetime__buf[fmt->n] = '\0';

    return datetime__from_strn(str, datetime__buf, fmt->n, tz, e);
}

/*
 * Returns `0` when failed (with `e` set), otherwise the number of chars
 * written inside the given buffer.
 */
static size_t datetime__write(
        ti_datetime_t * dt,
        char * buf,
        size_t buf_sz,
        const char * fmt,
        ex_t * e)
{
    size_t sz;
    struct tm tm = {0};
    if (ti_datetime_time(dt, &tm))
    {
        ex_set(e, EX_VALUE_ERROR, "failed to localize time");
        return 0;
    }

    sz = strftime(buf, buf_sz, fmt, &tm);
    if (sz == 0)
        ex_set(e, EX_VALUE_ERROR, "invalid date/time template");
    return sz;
}

/*
 * This function is not thread safe.
 */
ti_raw_t * ti_datetime_to_str(ti_datetime_t * dt, ex_t * e)
{
    ti_raw_t * str;
    size_t sz = datetime__write(
            dt,
            datetime__buf,
            DATETIME__BUF_SZ,
            datetime__fmt(dt),
            e);
    if (!sz)
        return NULL;  /* e is set */

    str = ti_str_create(datetime__buf, sz);
    if (!str)
        ex_set_mem(e);
    return str;
}

ti_raw_t * ti_datetime_to_str_fmt(ti_datetime_t * dt, ti_raw_t * fmt, ex_t * e)
{
    size_t sz;
    ti_raw_t * str;
    const size_t buf_sz = 120;
    char buf[buf_sz];

    if (fmt->n < 2|| fmt->n >= DATETIME__BUF_SZ)
    {
        ex_set(e, EX_VALUE_ERROR, "invalid date/time format (wrong size)");
        return NULL;
    }

    memcpy(&datetime__buf, fmt->data, fmt->n);
    datetime__buf[fmt->n] = '\0';

    sz = datetime__write(
            dt,
            buf,
            buf_sz,
            datetime__buf,
            e);
    if (!sz)
        return NULL;  /* e is set */

    str = ti_str_create(buf, sz);
    if (!str)
        ex_set_mem(e);

    return str;
}


/*
 * This function is not thread safe.
 */
int ti_datetime_to_pk(ti_datetime_t * dt, msgpack_packer * pk, int options)
{
    if (options >= 0)
    {
        if (ti_datetime_is_datetime(dt))
        {
            /* pack client result, convert to string */
            ex_t e = {0};
            size_t sz = datetime__write(
                    dt,
                    datetime__buf,
                    DATETIME__BUF_SZ,
                    datetime__fmt(dt),
                    &e);
            return sz
                ? mp_pack_strn(pk, datetime__buf, sz)
                : mp_pack_str(pk, "1970-01-01T00:00:00Z");
        }
        return msgpack_pack_int64(pk, dt->ts);
    }

    return (
        msgpack_pack_map(pk, 1) ||
        mp_pack_strn(
                pk,
                ti_datetime_is_timeval(dt)
                    ? TI_KIND_S_TIMEVAL
                    : TI_KIND_S_DATETIME,
                1) ||
        msgpack_pack_array(pk, 3) ||
        msgpack_pack_int64(pk, dt->ts) ||
        msgpack_pack_fix_int16(pk, dt->offset) ||
        (dt->tz ? msgpack_pack_uint64(pk, dt->tz->index) : msgpack_pack_nil(pk))
    );
}

int ti_datetime_to_zone(ti_datetime_t * dt, ti_raw_t * tzinfo, ex_t * e)
{
    if (!tzinfo->n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "expecting a time zone or offset; "
                "example time zone: `Europe/Amsterdam`; example offset: `+01`");
        return e->nr;
    }

    if (*tzinfo->data == '+' || *tzinfo->data == '-')
    {
        long int offset;
        const size_t buf_sz = 6;
        char buf[buf_sz];
        if (tzinfo->n >= buf_sz)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid offset; "
                    "expecting format `+/-hh[mm]`, for example +0100 or -05");
            return e->nr;
        }

        memcpy(buf, tzinfo->data, tzinfo->n);
        buf[tzinfo->n] = '\0';
        offset = datetime__offset_in_sec(buf, e);
        if (e->nr)
            return e->nr;

        offset /= 60;

        dt->offset = offset;
        dt->tz = NULL;
    }
    else
    {
        ti_tz_t * tz = ti_tz_from_strn((const char *) tzinfo->data, tzinfo->n);
        if (!tz)
        {
            ex_set(e, EX_VALUE_ERROR, "unknown time zone");
            return e->nr;
        }

        dt->offset = 0;
        dt->tz = tz;
    }

    return e->nr;
}

ti_datetime_t * ti_datetime_from_tm_tz(struct tm * tm, ti_tz_t * tz, ex_t * e)
{
    time_t ts;
    ti_datetime_t * dt;
    long int offset;
    int mday = tm->tm_mday;

    if (tz)
    {
        /* get offset after calculating time stamp */
        ti_tz_set(tz);
        ts = mktime(tm);
        offset = tm->tm_gmtoff / 60;
    }
    else
    {
        /* get offset before calculating time stamp */
        offset = tm->tm_gmtoff;
        ti_tz_set_utc();
        ts = mktime(tm) - offset;
        offset /= 60;
    }

    if (tm->tm_mday != mday)
    {
        ex_set(e, EX_VALUE_ERROR, "day %d is out of range for month", mday);
        return NULL;
    }

    dt = ti_datetime_from_i64(ts, (int16_t) offset, tz);
    if (!dt)
        ex_set_mem(e);
    return dt;
}

ti_datetime_t * ti_datetime_from_tm_tzinfo(
        struct tm * tm,
        ti_raw_t * tzinfo,
        ex_t * e)
{
    ti_datetime_t * dt;
    ti_tz_t * tz;
    time_t ts;
    long int offset;
    int mday = tm->tm_mday;

    if (!tzinfo->n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "expecting a time zone or offset; "
                "example time zone: `Europe/Amsterdam`; example offset: `+01`");
        return NULL;
    }

    if (*tzinfo->data == '+' || *tzinfo->data == '-')
    {
        const size_t buf_sz = 6;
        char buf[buf_sz];
        if (tzinfo->n >= buf_sz)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid offset; "
                    "expecting format `+/-hh[mm]`, for example +0100 or -05");
            return NULL;
        }

        memcpy(buf, tzinfo->data, tzinfo->n);
        buf[tzinfo->n] = '\0';

        offset = datetime__offset_in_sec(buf, e);
        if (e->nr)
            return NULL;

        ti_tz_set_utc();
        ts = mktime(tm) - offset;
        offset /= 60;
        tz = NULL;
    }
    else
    {
        tz = ti_tz_from_strn((const char *) tzinfo->data, tzinfo->n);
        if (!tz)
        {
            ex_set(e, EX_VALUE_ERROR, "unknown time zone");
            return NULL;
        }

        ti_tz_set(tz);
        ts = mktime(tm);
        offset = tm->tm_gmtoff / 60;
    }

    if (tm->tm_mday != mday)
    {
        ex_set(e, EX_VALUE_ERROR, "day %s is out of range for month", mday);
        return NULL;
    }

    dt = ti_datetime_from_i64(ts, (int16_t) offset, tz);
    if (!dt)
        ex_set_mem(e);
    return dt;
}

static void datetime__correct_max_mday(struct tm * tm)
{
    int mday = tm->tm_mday;
    int month = tm->tm_mon;

    if (month == 3 || month == 5 || month == 8 || month == 10)
    {
        /* April, June, September, November */
        if (mday > 30)
            tm->tm_mday = 30;
    }
    else if (month == 1)
    {
        /* February */
        long int y = tm->tm_year + 1900;
        int mfeb = 28 + ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
        if (mday > mfeb)
            tm->tm_mday = mfeb;
    }
    else if (mday > 31)
    {
        /* January, March, May, July, August, October, December */
        tm->tm_mday = 31;
    }
}

int ti_datetime_move(
        ti_datetime_t * dt,
        datetime_unit_e unit,
        int64_t num,
        ex_t * e)
{
    if (unit <= DT_MONTHS)
    {
        struct tm tm = {0};
        if (ti_datetime_time(dt, &tm))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "failed to localize time");
            return e->nr;
        }

        switch(unit)
        {
        case DT_MONTHS:
            if ((num > 0 && tm.tm_mon > INT_MAX - num) ||
                (num < 0 && tm.tm_mon < INT_MIN - num))
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            tm.tm_mon += num;
            num = tm.tm_mon / 12;
            tm.tm_mon %= 12;
            if (tm.tm_mon < 0)
            {
                tm.tm_mon += 12;
                num -= 1;
            }
            /* fall through */
        case DT_YEARS:
            if ((num > 0 && tm.tm_year > INT_MAX - num) ||
                (num < 0 && tm.tm_year < INT_MIN - num))
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                return e->nr;
            }
            tm.tm_year += num;
            break;
        default:
            assert(0);
            ex_set_internal(e);
            return e->nr;
        }

        /* ensures the the day of the month is correct */
        if (tm.tm_mday > 28)
            datetime__correct_max_mday(&tm);

        dt->ts = mktime(&tm);
        return 0;
    }
    else
    {
        if (num < INT_MIN || num > INT_MAX)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        switch(unit)
        {
        case DT_YEARS:
        case DT_MONTHS:
            assert(0);
            /* fall through */
        case DT_WEEKS:
            num *= 7;
            /* fall through */
        case DT_DAYS:
            num *= 24;
            /* fall through */
        case DT_HOURS:
            num *= 60;
            /* fall through */
        case DT_MINUTES:
            num *= 60;
            /* fall through */
        case DT_SECONDS:
            break;
        }

        if ((num > 0 && dt->ts > LLONG_MAX - num) ||
            (num < 0 && dt->ts < LLONG_MIN - num))
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            return e->nr;
        }

        dt->ts += num;
    }

    return e->nr;
}

int ti_datetime_weekday(ti_datetime_t * dt)
{
    struct tm tm = {0};
    return ti_datetime_time(dt, &tm) < 0 ? -1 : tm.tm_wday;
}

int ti_datetime_yday(ti_datetime_t * dt)
{
    struct tm tm = {0};
    return ti_datetime_time(dt, &tm) < 0 ? -1 : tm.tm_yday;
}

int ti_datetime_week(ti_datetime_t * dt)
{
    struct tm tm = {0};
    return ti_datetime_time(dt, &tm) < 0
            ? -1                                    /* error */
            : tm.tm_yday < tm.tm_wday
            ? 0                                     /* before first Sunday */
            : (tm.tm_yday - tm.tm_wday) / 7 + 1;    /* calculate week */
}

const char * ti_datetime_ts_str(const time_t * ts)
{
    struct tm tm;

    if (gmtime_r(ts, &tm) != &tm)
        return "error date/time";

    (void) strftime(
            datetime__buf,
            DATETIME__BUF_SZ,
            datetime__fmp_utc,
            &tm);
    return datetime__buf;
}

/*
 * Returns the unit corresponding to the given string.
 * (`e` must be checked for errors)
 */
datetime_unit_e ti_datetime_get_unit(ti_raw_t * raw, ex_t * e)
{
    if (ti_raw_eq_strn(raw, "years", 5))
        return DT_YEARS;
    if (ti_raw_eq_strn(raw, "months", 6))
        return DT_MONTHS;
    if (ti_raw_eq_strn(raw, "weeks", 5))
        return DT_WEEKS;
    if (ti_raw_eq_strn(raw, "days", 4))
        return DT_DAYS;
    if (ti_raw_eq_strn(raw, "hours", 5))
        return DT_HOURS;
    if (ti_raw_eq_strn(raw, "minutes", 7))
        return DT_MINUTES;
    if (ti_raw_eq_strn(raw, "seconds", 7))
        return DT_SECONDS;

    ex_set(e, EX_VALUE_ERROR,
            "invalid unit, "
            "expecting one of `years`, `months`, `weeks`, `days`, "
            "`hours`, `minutes` or `seconds`");
    return 0;
}

