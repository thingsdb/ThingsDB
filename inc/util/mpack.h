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
    MP_UNSUPPORTED=-3,
    MP_INCOMPLETE=-2,
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

static inline const char * __attribute__((unused))mp_type_str(mp_enum_t tp)
{
    switch (tp)
    {
    case MP_UNSUPPORTED:        return "unsupported msgpack type";
    case MP_INCOMPLETE:         return "msgpack data is incomplete";
    case MP_ERR:                return "unexpected msgpack error";
    case MP_END:                return "end of msgpack data";
    case MP_I64:                return "int64";
    case MP_U64:                return "uint64";
    case MP_F64:                return "float64";
    case MP_BIN:                return "binary";
    case MP_STR:                return "utf8";
    case MP_BOOL:               return "boolean";
    case MP_NIL:                return "nil";
    case MP_ARR:                return "array";
    case MP_MAP:                return "map";
    }
    return "msgpack type unknown";
}

typedef struct
{
    char * pt;
    char * end;
} mp_unp_t;

typedef struct
{
    mp_enum_t tp;
    mp_via_t via;
} mp_obj_t;

static inline int mp_pack_bin(msgpack_packer * x, const void * b, size_t n)
{
    return msgpack_pack_bin(x, n) || msgpack_pack_bin_body(x, b, n);
}

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

#define mp_read_8(tp__, addr__, up__) \
do { \
    if (up__->pt >= up__->end) \
        return MP_INCOMPLETE; \
    memcpy(addr__, up__->pt, sizeof(tp__)); \
    ++up__->pt; \
} while(0)

#define mp_read_16(tp__, addr__, up__) \
do { \
    if (up__->pt + 1 >= up__->end) \
        return MP_INCOMPLETE; \
    _msgpack_load16(tp__, up__->pt, addr__); \
    up__->pt += 2; \
} while(0)

#define mp_read_32(tp__, addr__, up__) \
do { \
    if (up__->pt + 3 >= up__->end) \
        return MP_INCOMPLETE; \
    _msgpack_load32(tp__, up__->pt, addr__); \
    up__->pt += 4; \
} while(0)

#define mp_read_64(tp__, addr__, up__) \
do { \
    if (up__->pt + 7 >= up__->end) \
        return MP_INCOMPLETE; \
    _msgpack_load64(tp__, up__->pt, addr__); \
    up__->pt += 8; \
} while(0)

#define mp_skip_chars(n__, up__) \
do { \
    up__->pt += n__; \
    if (up__->pt > up__->end) \
        return MP_INCOMPLETE; \
} while(0)

#define mp_skip_n(n__, up__) \
do { \
    mp_enum_t tp__; \
    size_t sz__ = n__; \
    while (sz__--) \
        if ((tp__ = mp_skip(up__)) <= 0) \
            return tp__ == 0 ? MP_INCOMPLETE : tp__; \
} while(0)

#define mp_read_obj_data(o__, up__) \
do { \
    o__->via.str.data = up__->pt; \
    up__->pt += o__->via.str.n; \
    if (up__->pt > up__->end) \
        return MP_INCOMPLETE; \
} while(0)

static mp_enum_t __attribute__((unused))mp_next(mp_unp_t * up, mp_obj_t * o)
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
        return (o->tp = MP_UNSUPPORTED);
    case 0xc2:              /* false */
        o->via.bool_ = false;
        return (o->tp = MP_BOOL);
    case 0xc3:              /* true */
        o->via.bool_ = true;
        return (o->tp = MP_BOOL);
    case 0xc4:              /* bin 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up);
        o->via.str.n = u8;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc5:              /* bin 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        o->via.str.n = u16;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc6:              /* bin 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        o->via.str.n = u32;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc7:              /* ext 8 */
    case 0xc8:              /* ext 16 */
    case 0xc9:              /* ext 32 */
        return MP_UNSUPPORTED;
    case 0xca:              /* float 32 */
    {
        union { float f; uint32_t u; } mem;
        mp_read_32(uint32_t, &mem.u, up);
        o->via.f64 = (double) mem.f;
        return (o->tp = MP_F64);
    }
    case 0xcb:              /* float 64 */
    {
        union { double d; uint64_t u; } mem;
        mp_read_64(uint64_t, &mem.u, up);
        o->via.f64 = mem.d;
        return (o->tp = MP_F64);
    }
    case 0xcc:              /* uint 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up);
        o->via.u64 = u8;
        return (o->tp = MP_U64);
    }
    case 0xcd:              /* uint 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        o->via.u64 = u16;
        return (o->tp = MP_U64);
    }
    case 0xce:              /* uint 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        o->via.u64 = u32;
        return (o->tp = MP_U64);
    }
    case 0xcf:              /* uint 64 */
    {
        uint64_t u64;
        mp_read_64(uint64_t, &u64, up);
        o->via.u64 = u64;
        return (o->tp = MP_U64);
    }
    case 0xd0:              /* int 8 */
    {
        int8_t i8;
        mp_read_8(int8_t, &i8, up);
        o->via.i64 = i8;
        return (o->tp = MP_I64);
    }
    case 0xd1:              /* int 16 */
    {
        int16_t i16;
        mp_read_16(int16_t, &i16, up);
        o->via.i64 = i16;
        return (o->tp = MP_I64);
    }
    case 0xd2:              /* int 32 */
    {
        int32_t i32;
        mp_read_32(int32_t, &i32, up);
        o->via.i64 = i32;
        return (o->tp = MP_I64);
    }
    case 0xd3:              /* int 64 */
    {
        int64_t i64;
        mp_read_64(int64_t, &i64, up);
        o->via.i64 = i64;
        return (o->tp = MP_I64);
    }
    case 0xd4:              /* fixext 1 */
    case 0xd5:              /* fixext 2 */
    case 0xd6:              /* fixext 4 */
    case 0xd7:              /* fixext 8 */
    case 0xd8:              /* fixext 16 */
        return MP_UNSUPPORTED;
    case 0xd9:              /* str 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up);
        o->via.str.n = u8;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xda:              /* str 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        o->via.str.n = u16;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdb:              /* str 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        o->via.str.n = u32;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdc:              /* array 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        o->via.sz = u16;
        return (o->tp = MP_ARR);
    }
    case 0xdd:              /* array 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        o->via.sz = u32;
        return (o->tp = MP_ARR);
    }
    case 0xde:              /* map 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        o->via.sz = u16;
        return (o->tp = MP_MAP);
    }
    case 0xdf:              /* map 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        o->via.sz = u32;
        return (o->tp = MP_MAP);
    }
    case 0xe0 ... 0xff:     /* fixed negative */
        o->via.i64 = (char) token;
        return (o->tp = MP_I64);
    }

    return MP_ERR;
}

static mp_enum_t __attribute__((unused))mp_skip(mp_unp_t * up)
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
        mp_skip_n((0xf & token) * 2, up);
        return MP_MAP;
    case 0x90 ... 0x9f:     /* fixed array */
        mp_skip_n(0xf & token, up);
        return MP_ARR;
    case 0xa0 ... 0xbf:     /* fixed str */
    {
        uint8_t n = 0x1f & token;
        mp_skip_chars(n, up);
        return MP_STR;
    }
    case 0xc0:              /* nil */
        return MP_NIL;
    case 0xc1:              /* never used */
        return MP_UNSUPPORTED;
    case 0xc2:              /* false */
    case 0xc3:              /* true */
        return MP_BOOL;
    case 0xc4:              /* bin 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up);
        mp_skip_chars(u8, up);
        return MP_BIN;
    }
    case 0xc5:              /* bin 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        mp_skip_chars(u16, up);
        return MP_BIN;
    }
    case 0xc6:              /* bin 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        mp_skip_chars(u32, up);
        return MP_BIN;
    }
    case 0xc7:              /* ext 8 */
    case 0xc8:              /* ext 16 */
    case 0xc9:              /* ext 32 */
        return MP_UNSUPPORTED;
    case 0xca:              /* float 32 */
        mp_skip_chars(4, up);
        return MP_F64;
    case 0xcb:              /* float 64 */
        mp_skip_chars(8, up);
        return MP_F64;
    case 0xcc:              /* uint 8 */
        mp_skip_chars(sizeof(uint8_t), up);
        return MP_U64;
    case 0xcd:              /* uint 16 */
        mp_skip_chars(sizeof(uint16_t), up);
        return MP_U64;
    case 0xce:              /* uint 32 */
        mp_skip_chars(sizeof(uint32_t), up);
        return MP_U64;
    case 0xcf:              /* uint 64 */
        mp_skip_chars(sizeof(uint64_t), up);
        return MP_U64;
    case 0xd0:              /* int 8 */
        mp_skip_chars(sizeof(int8_t), up);
        return MP_I64;
    case 0xd1:              /* int 16 */
        mp_skip_chars(sizeof(int16_t), up);
        return MP_I64;
    case 0xd2:              /* int 32 */
        mp_skip_chars(sizeof(int32_t), up);
        return MP_I64;
    case 0xd3:              /* int 64 */
        mp_skip_chars(sizeof(int64_t), up);
        return MP_I64;
    case 0xd4:              /* fixext 1 */
    case 0xd5:              /* fixext 2 */
    case 0xd6:              /* fixext 4 */
    case 0xd7:              /* fixext 8 */
    case 0xd8:              /* fixext 16 */
        return MP_UNSUPPORTED;
    case 0xd9:              /* str 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up);
        mp_skip_chars(u8, up);
        return MP_STR;
    }
    case 0xda:              /* str 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        mp_skip_chars(u16, up);
        return MP_STR;
    }
    case 0xdb:              /* str 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        mp_skip_chars(u32, up);
        return MP_STR;
    }
    case 0xdc:              /* array 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        mp_skip_n(u16, up);
        return MP_ARR;
    }
    case 0xdd:              /* array 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        mp_skip_n(u32, up);
        return MP_ARR;
    }
    case 0xde:              /* map 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up);
        mp_skip_n(u16*2, up);
        return MP_MAP;
    }
    case 0xdf:              /* map 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up);
        mp_skip_n(u32*2, up);
        return MP_MAP;
    }
    case 0xe0 ... 0xff:     /* fixed negative */
        return MP_I64;
    }

    return MP_ERR;
}

static inline int mp_pack_append(msgpack_packer * pk, const void * s, size_t n)
{
    msgpack_pack_append_buffer(pk, s, n);
}

static inline void * mp_may_cast_u64(mp_enum_t * tp)
{
    return tp == MP_I64 || tp == MP_U64;
}

static inline void * mp_strdup(mp_obj_t * o)
{
    assert (o->tp == MP_STR || o->tp == MP_BIN);
    return strndup(o->via.str.data, o->via.str.n);
}

#endif  /* MPACK_H_ */
