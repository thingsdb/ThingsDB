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
#include <util/mpack.h>

struct ti_datetime_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t offset;        /* offset in minutes */
    time_t ts;              /* time-stamp in seconds */
};

ti_datetime_t * ti_datetime_from_i64(int64_t ts, int16_t offset);
ti_datetime_t * ti_datetime_from_double(double d, int16_t offset, ex_t * e);
ti_raw_t * ti_datetime_to_str(ti_datetime_t * dt, ex_t * e);
int ti_datetime_to_pk(ti_datetime_t * dt, msgpack_packer * pk, int options);

#endif  /* TI_DATETIME_H_ */
