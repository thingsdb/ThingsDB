/*
 * util/mpack.h
 */
#define _GNU_SOURCE

#ifndef MPACK_H_
#define MPACK_H_

#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <util/logger.h>

typedef struct
{
    unsigned char * data;
    size_t n;
} mp_bin_t;

/* use only when UTF8 is guaranteed */
typedef struct
{
    char * data;
    size_t n;
} mp_str_t;

typedef union
{
    int64_t i64;
    uint64_t u64;
    double f64;
    mp_bin_t bin;
    mp_str_t str;
    _Bool bool_;
    size_t sz;
} mp_via_t;

typedef enum
{
    MP_ERR=-1,
    MP_END,     /* end of data reached */
    MP_I64,
    MP_U64,
    MP_F64,
    MP_BIN,
    MP_STR,
    MP_BOOL,
    MP_NIL,
    MP_ARR,
    MP_MAP,
} mp_enum_t;

typedef struct
{
    char * begin;
    char * pt;
    char * end;
} mp_unp_t;

typedef struct
{
    mp_enum_t tp;
    mp_via_t via;
} mp_obj_t;


static inline int mp_pack_strn(msgpack_packer * x, const void * s, size_t n)
{
    return msgpack_pack_str(x, n) || msgpack_pack_str_body(x, s, n);
}

static inline int mp_pack_str(msgpack_packer * x, const void * s)
{
    size_t n = strlen(s);
    return mp_pack_strn(x, s, n);
}

static inline void mp_unp_init(mp_unp_t * up, void * data, size_t n)
{
    up->pt = data;
    up->end = up->pt + n;
}

#define mp_read_u8(u8addr__, up__) \
do { \
    if (up__->pt >= up__->end) \
        return MP_ERR; \
    memcpy(u8addr__, up__->pt, sizeof(uint8_t)); \
    ++up__->pt; \
} while(0)

#define mp_read_u16(u16addr__, up__) \
do { \
    if (up__->pt + 1 >= up__->end) \
        return MP_ERR; \
    _msgpack_load16(uint16_t, up__->pt, u16addr__); \
    up__->pt += 2; \
} while(0)

#define mp_read_u32(u32addr__, up__) \
do { \
    if (up__->pt + 3 >= up__->end) \
        return MP_ERR; \
    _msgpack_load32(uint32_t, up__->pt, u32addr__); \
    up__->pt += 4; \
} while(0)

#define mp_read_u64(u64addr__, up__) \
do { \
    if (up__->pt + 7 >= up__->end) \
        return MP_ERR; \
    _msgpack_load64(uint64_t, up__->pt, u64addr__); \
    up__->pt += 8; \
} while(0)

#define mp_read_obj_data(o__, up__) \
do { \
    o__->via.str.data = up__->pt; \
    up__->pt += o__->via.str.n; \
    if (up__->pt <= up__->end) \
        return MP_ERR; \
} while(0)

static mp_enum_t mp_next(mp_unp_t * up, mp_obj_t * o)
{
    if (up->pt >= up->end)
        return MP_END;

    unsigned char token = (unsigned char) *up->pt;

    ++up->pt;

    switch (token)
    {
    case 0x00 ... 0x7f:     /* fixed positive */
        o->via.u64 = token;
        return (o->tp = MP_U64);
    case 0x80 ... 0x8f:     /* fixed map */
        o->via.sz = 0xf & token;
        return (o->tp = MP_MAP);
    case 0x90 ... 0x9f:     /* fixed array */
        o->via.sz = 0xf & token;
        return (o->tp = MP_ARR);
    case 0xa0 ... 0xbf:     /* fixed str */
        o->via.str.n = 0x1f & token;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    case 0xc0:              /* nil */
        return (o->tp = MP_NIL);
    case 0xc1:              /* never used */
        return (o->tp = MP_ERR);
    case 0xc2:              /* false */
        o->via.bool_ = false;
        return (o->tp = MP_BOOL);
    case 0xc3:              /* true */
        o->via.bool_ = true;
        return (o->tp = MP_BOOL);
    case 0xc4:              /* bin 8 */
    case 0xc5:              /* bin 16 */
    case 0xc6:              /* bin 32 */
    case 0xc7:              /* ext 8 */
    case 0xc8:              /* ext 16 */
    case 0xc9:              /* ext 32 */
    case 0xca:              /* float 32 */
    case 0xcb:              /* float 64 */
    case 0xcc:              /* uint 8 */
    {
        uint8_t u8;
        mp_read_u8(&u8, up);
        o->via.u64 = u8;
        return (o->tp = MP_U64);
    }
    case 0xcd:              /* uint 16 */
    {
        uint16_t u16;
        mp_read_u16(&u16, up);
        o->via.u64 = u16;
        return (o->tp = MP_U64);
    }
    case 0xce:              /* uint 32 */
    {
        uint32_t u32;
        mp_read_u32(&u32, up);
        o->via.u64 = u32;
        return (o->tp = MP_U64);
    }
    case 0xcf:              /* uint 64 */
    {
        uint64_t u64;
        mp_read_u64(&u64, up);
        o->via.u64 = u64;
        return (o->tp = MP_U64);
    }
    case 0xd0:              /* int 8 */
    case 0xd1:              /* int 16 */
    case 0xd2:              /* int 32 */
    case 0xd3:              /* int 64 */
    case 0xd4:              /* fixext 1 */
    case 0xd5:              /* fixext 2 */
    case 0xd6:              /* fixext 4 */
    case 0xd7:              /* fixext 8 */
    case 0xd8:              /* fixext 16 */
    case 0xd9:              /* str 8 */
    {
        uint8_t u8;
        mp_read_u8(&u8, up);
        o->via.str.n = u8;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xda:              /* str 16 */
    {
        uint16_t u16;
        mp_read_u16(&u16, up);
        o->via.str.n = u16;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdb:              /* str 32 */
    {
        uint32_t u32;
        mp_read_u32(&u32, up);
        o->via.str.n = u32;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdc:              /* array 16 */
    case 0xdd:              /* array 32 */
    case 0xde:              /* map 16 */
    case 0xdf:              /* map 32 */
    case 0xe0 ... 0xff:     /* fixed negative */
        o->via.i64 = (char) token;
        return (o->tp = MP_I64);
    }

    return MP_ERR;
}

static mp_enum_t mp_skip(mp_unp_t * up)
{
    if (up->pt >= up->end)
        return MP_END;

    unsigned char token = (unsigned char) *up->pt;

    ++up->pt;

    switch (token)
    {
    case 0x00 ... 0x7f:     /* fixed positive */
        return MP_U64;
    case 0x80 ... 0x8f:     /* fixed map */
        return MP_MAP;
    case 0x90 ... 0x9f:     /* fixed array */
        return MP_ARR;
    case 0xa0 ... 0xbf:     /* fixed str */
    {
        size_t n = 0x1f & token;
        up->pt += n;
        return up->pt <= up->end ? MP_STR : MP_ERR;
    }
    case 0xc0:              /* nil */
        return MP_NIL;
    case 0xc1:              /* never used */
        return MP_ERR;
    case 0xc2:              /* false */
    case 0xc3:              /* true */
        return MP_BOOL;
    case 0xc4:              /* bin 8 */
    case 0xc5:              /* bin 16 */
    case 0xc6:              /* bin 32 */
    case 0xc7:              /* ext 8 */
    case 0xc8:              /* ext 16 */
    case 0xc9:              /* ext 32 */
    case 0xca:              /* float 32 */
    case 0xcb:              /* float 64 */
    case 0xcc:              /* uint 8 */
    case 0xcd:              /* uint 16 */
    case 0xce:              /* uint 32 */
    case 0xcf:              /* uint 64 */
    case 0xd0:              /* int 8 */
    case 0xd1:              /* int 16 */
    case 0xd2:              /* int 32 */
    case 0xd3:              /* int 64 */
    case 0xd4:              /* fixext 1 */
    case 0xd5:              /* fixext 2 */
    case 0xd6:              /* fixext 4 */
    case 0xd7:              /* fixext 8 */
    case 0xd8:              /* fixext 16 */
    case 0xd9:              /* str 8 */
    case 0xda:              /* str 16 */
    case 0xdb:              /* str 32 */
    case 0xdc:              /* array 16 */
    case 0xdd:              /* array 32 */
    case 0xde:              /* map 16 */
    case 0xdf:              /* map 32 */
    case 0xe0 ... 0xff:     /* fixed negative */
        return MP_I64;
    }

    return MP_ERR;
}

#endif  /* MPACK_H_ */
