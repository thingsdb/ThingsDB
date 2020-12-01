/*
 * ti/datetime.c
 */
#define _GNU_SOURCE
#include <ti/datetime.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Arguments `ts` is in seconds, `offset` in minutes.
 */
ti_datetime_t * ti_datetime_from_i64(int64_t ts, int16_t offset)
{
    ti_datetime_t * dt = malloc(sizeof(ti_datetime_t));
    if (!dt)
        return NULL;
    dt->ts = (time_t) ts;
    dt->offset = offset;
    return dt;
}

static ti_datetime_t * datetime__from_str(,
        ti_raw_t * str,
        const char * fmt,
        _Bool with_offset,
        ex_t * e)
{
    ti_datetime_t * dt;
    time_t ts;
    long int offset = 0;
    struct tm tm;
    static size_t buf_sz = 80;
    static char * buf[buf_sz];
    char * c;

    if (str->n >= buf_sz)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid datetime string (too large)");
        return NULL;
    }

    memcpy(buf, str->data, str->n);
    buf[str->n] = '\0';

    memset(&tm, 0, sizeof(struct tm));
    tm.tm_mday = 1;  /* must be set to 1 */

    c = strptime(buf, fmt, &tm);
    if (c == NULL || !*c)
        goto invalid;

    ts = mktime(&tm);

    if (with_offset)
    {
        size_t n = str->n;
        int sign = 0;

        do switch (buf[--n])
        {
        case '-':
            sign = -1;
            break;
        case '+':
            sign = 1;
        }
        while (n);

        if (!sign)
            goto invalid;

        if (!isdigit(buf[++n]))
            goto invalid;

        offset = (buf[n] - '0') * 36000;

        if (!isdigit(buf[++n]))
            goto invalid;

        offset += (buf[n] - '0') * 3600;

        if (isdigit(buf[++n]))
        {
            /* read minutes */
            offset += (buf[n] - '0') * 600;

            if (!isdigit(buf[++n]))
                goto invalid;

            offset += (buf[n] - '0') * 60;

            ++n;
        }

        if (buf[n] != '\0')
            goto invalid;

        offset *= sign;

        if ((offset < 0 && ts > LLONG_MAX + offset) ||
            (offset > 0 && ts < LLONG_MIN + offset))
        {
            ex_set(e, EX_OVERFLOW, "datetime overflow");
            return 0;
        }

        ts -= offset;
        offset /= 60;

        if (offset < DATETIME_OFFSET_MIN || offset > DATETIME_OFFSET_MAX)
            goto invalid;
    }

    dt = malloc(sizeof(ti_datetime_t));
    if (!dt)
    {
        ex_set_mem(e);
        return NULL;
    }

    dt->ts = ts;
    dt->offset = (int16_t) offset;  /* offset is checked */

    return dt;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid datetime string (does not match the format string)");
    return NULL;
}

static inline _Bool datetime__with_t(ti_raw_t * str)
{
    return str->data[10] == 'T';
}

ti_datetime_t * ti_datetime_from_str(ti_raw_t * str, ex_t * e)
{
    switch(str->n)
    {
    case 4:  /* YYYY */
        return datetime__from_str(str, "%Y", 0, e);
    case 7:  /* YYYY-MM */
        return datetime__from_str(str, "%Y-%m", 0, e);
    case 10: /* YYYY-MM-DD */
        return datetime__from_str(str, "%Y-%m-%d", 0, e);
    case 13: /* YYYY-MM-DDTHH */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H", 0, e);
    case 16: /* YYYY-MM-DDTHH:MM */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M", 0, e);
    case 19: /* YYYY-MM-DDTHH:MM:SS */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S", 0, e);
    case 22: /* YYYY-MM-DDTHH:MM:SS+hh */
    case 24: /* YYYY-MM-DDTHH:MM:SS+hhmm */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S%z", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S%z", 0, e);
    }

    ex_set(e, EX_VALUE_ERROR,
            "invalid datetime string");
    return NULL;
}

ti_datetime_t * ti_datetime_from_fmt(ti_raw_t * str, ti_raw_t * fmt, ex_t * e)
{
    static size_t buf_sz = 30;
    static char * buf[buf_sz];
    _Bool with_offset;

    if (fmt->n < 2 || fmt->n >= buf_sz)
    {
        ex_set(e, EX_VALUE_ERROR, "invalid datetime format (wrong size)");
        return NULL;
    }

    with_offset = fmt->data[fmt->n-2] == '%' && fmt->data[fmt->n-2] == 'z';

    memcpy(buf, fmt->data, fmt->n);
    buf[fmt->n] = '\0';

    return datetime__from_str(str, buf, with_offset, e);
}

/*
 * Returns `0` when failed (with `e` set), otherwise the number of chars
 * written inside the given buffer.
 */
static size_t datetime__get_buf(
        ti_datetime_t * dt,
        char * buf,
        size_t buf_sz,
        const char * fmt,
        ex_t * e)
{
    size_t sz;
    struct tm tm = {0};
    long int offset = dt->offset * 60;
    time_t ts = dt->ts;

    if ((offset > 0 && ts > LLONG_MAX - offset) ||
        (offset < 0 && ts < LLONG_MIN - offset))
    {
        ex_set(e, EX_OVERFLOW, "datetime overflow");
        return 0;
    }

    ts += offset;

    if (localtime_r(&ts, &tm) != &tm)
    {
        ex_set(e, EX_VALUE_ERROR, "failed to localize datetime object");
        return 0;
    }

    tm.tm_gmtoff = offset;

    sz = strftime(buf, buf_sz, fmt, &tm);
    if (sz == 0)
        ex_set(e, EX_VALUE_ERROR, "invalid datetime template");
    return sz;
}

/*
 * This function is not thread safe.
 */
ti_raw_t * ti_datetime_to_str(ti_datetime_t * dt, ex_t * e)
{
    ti_raw_t * str;
    static size_t buf_sz = 32;
    static char * buf[buf_sz];
    static const char * fo = "%Y-%m-%dT%H:%M:%S%z";
    static const char * fu = "%Y-%m-%dT%H:%M:%SZ";
    size_t sz = datetime__get_buf(dt, &buf, buf_sz, dt->offset ? fo : fu, e);
    if (!sz)
        return NULL;

    str = ti_str_create(&buf, sz);
    if (!str)
        ex_set_mem(e);
    return str;
}

int ti_datetime_to_pk(ti_datetime_t * dt, msgpack_packer * pk, int options)
{
    if (options >= 0)
    {
        /* pack client result, convert to string */
    }
    else
    {
        /* pack internal/event, pack values */
    }
}

