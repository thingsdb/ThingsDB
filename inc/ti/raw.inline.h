/*
 * ti/raw.inline.h
 */
#ifndef TI_RAW_INLINE_H_
#define TI_RAW_INLINE_H_

#include <ti/raw.h>
#include <ti/val.h>
#include <util/mpack.h>

static inline ti_raw_t * ti_str_create(const char * str, size_t n)
{
    return ti_raw_create(TI_VAL_STR, str, n);
}
static inline ti_raw_t * ti_str_from_str(const char * str)
{
    return ti_raw_create(TI_VAL_STR, str, strlen(str));
}

static inline ti_raw_t * ti_bin_create(const unsigned char * bin, size_t n)
{
    return ti_raw_create(TI_VAL_BYTES, bin, n);
}
static inline ti_raw_t * ti_mp_create(const unsigned char * bin, size_t n)
{
    return ti_raw_create(TI_VAL_MPDATA, bin, n);
}

static inline _Bool ti_raw_is_name(ti_raw_t * raw)
{
    return raw->tp == TI_VAL_NAME;
}

static inline _Bool ti_raw_is_reserved_key(ti_raw_t * raw)
{
    return raw->n == 1 && (*raw->data >> 4 == 2);
}

static inline int ti_raw_bytes_to_pk(ti_raw_t * raw, msgpack_packer * pk)
{
    return mp_pack_bin(pk, raw->data, raw->n); \
}

static inline int ti_raw_str_to_pk(ti_raw_t * raw, msgpack_packer * pk)
{
    return mp_pack_strn(pk, raw->data, raw->n); \
}

static inline int ti_raw_mpdata_to_pk(
        ti_raw_t * raw,
        msgpack_packer * pk,
        int options)
{
    return options >= 0
        ? mp_pack_append(pk, raw->data, raw->n)
        : mp_pack_ext(pk, MPACK_EXT_MPACK, raw->data, raw->n);
}

#endif  /* TI_RAW_INLINE_H_ */
