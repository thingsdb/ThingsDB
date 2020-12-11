/*
 * ti/tz.h
 */
#ifndef TI_TZ_H_
#define TI_TZ_H_

#include <stddef.h>
#include <ti/val.t.h>
typedef struct ti_tz_s ti_tz_t;

struct ti_tz_s
{
    char * name;    /* null terminated string */
    size_t n;       /* size excluding null terminator */
    size_t index;   /* index number */
};

void ti_tz_init(void);
void ti_tz_set_utc(void);
void ti_tz_set(ti_tz_t * tz);
ti_tz_t * ti_tz_utc(void);
ti_tz_t * ti_tz_from_index(size_t tz_index);
ti_tz_t * ti_tz_from_strn(register const char * s, register size_t n);

static inline _Bool ti_tz_is_not_utc(ti_tz_t * tz)
{
    return tz && tz->index;
}

ti_val_t * ti_tz_as_mpval(void);

#endif  /* TI_TZ_H_ */
