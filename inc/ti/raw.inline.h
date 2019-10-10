/*
 * ti/raw.inline.h
 */
#ifndef TI_RAW_INLINE_H_
#define TI_RAW_INLINE_H_

#include <ti/raw.h>
#include <ti/val.h>

static inline ti_raw_t * ti_str_create(const char * str, size_t n)
{
    return ti_raw_create(TI_VAL_STR, str, n);
}
static inline ti_raw_t * ti_bin_create(const unsigned char * bin, size_t n)
{
    return ti_raw_create(TI_VAL_BYTES, bin, n);
}
static inline ti_raw_t * ti_mp_create(const unsigned char * bin, size_t n)
{
    return ti_raw_create(TI_VAL_MP, bin, n);
}

#endif  /* TI_RAW_INLINE_H_ */
