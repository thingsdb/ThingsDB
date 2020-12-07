/*
 * ti/datetime.h
 */
#ifndef TI_DATETIME_H_
#define TI_DATETIME_H_

typedef struct ti_datetime_s ti_datetime_t;

#define DATETIME(__x)  ((ti_datetime_t *) (__x))->ts
#define DATETIME_OFFSET_MIN (-720)
#define DATETIME_OFFSET_MAX (+720)

#include <inttypes.h>
#include <time.h>
#include <ex.h>
#include <ti/raw.h>
#include <ti/tz.h>
#include <util/mpack.h>

typedef enum
{
    DT_YEARS,
    DT_MONTHS,
    DT_WEEKS,
    DT_DAYS,
    DT_HOURS,
    DT_MINUTES,
    DT_SECONDS,
} datetime_unit_e;

struct ti_datetime_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int16_t offset;         /* offset in minutes */
    time_t ts;              /* time-stamp in seconds */
    ti_tz_t * tz;           /* may be NULL */
};

ti_datetime_t * ti_datetime_from_i64(int64_t ts, int16_t offset, ti_tz_t * tz);
ti_datetime_t * ti_datetime_copy(ti_datetime_t * dt);
ti_datetime_t * ti_datetime_from_str(ti_raw_t * str, ti_tz_t * tz, ex_t * e);
ti_datetime_t * ti_datetime_from_fmt(
        ti_raw_t * str,
        ti_raw_t * fmt,
        ti_tz_t * tz,
        ex_t * e);
ti_datetime_t * ti_datetime_from_tm_tz(
        struct tm * tm,
        ti_tz_t * tz,
        ex_t * e);
ti_datetime_t * ti_datetime_from_tm_tzinfo(
        struct tm * tm,
        ti_raw_t * tzinfo,
        ex_t * e);
ti_raw_t * ti_datetime_to_str(ti_datetime_t * dt, ex_t * e);
ti_raw_t * ti_datetime_to_str_fmt(ti_datetime_t * dt, ti_raw_t * fmt, ex_t * e);
int ti_datetime_to_pk(ti_datetime_t * dt, msgpack_packer * pk, int options);
int ti_datetime_to_zone(ti_datetime_t * dt, ti_raw_t * tzinfo, ex_t * e);
void ti_datetime_set_tz(ti_tz_t * tz);
int ti_datetime_move(
        ti_datetime_t * dt,
        datetime_unit_e unit,
        int64_t num,
        ex_t * e);
datetime_unit_e ti_datetime_get_unit(ti_raw_t * raw, ex_t * e);
int ti_datetime_weekday(ti_datetime_t * dt);
int ti_datetime_yday(ti_datetime_t * dt);
int ti_datetime_week(ti_datetime_t * dt);

#endif  /* TI_DATETIME_H_ */
