/*
 * ti/tz.h
 */
#ifndef TI_TZ_H_
#define TI_TZ_H_

typedef struct ti_tz_s ti_tz_t;

struct ti_tz_s
{
    char * name;    /* null terminated string */
    size_t n;       /* size excluding null terminator */
    uint16_t index; /* index number */
};

void ti_tz_init(void);
ti_tz_t * ti_tz_utc();
ti_tz_t * ti_tz_from_i64(int64_t * tz_index);


#endif  /* TI_TZ_H_ */
