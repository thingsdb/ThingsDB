/*
 * util/mpack.h
 *
 * TESTED with msgpack version: 3.2.0
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef MPACK_H_
#define MPACK_H_

#include <msgpack.h>
#include <inttypes.h>
#include <msgpack/fbuffer.h>
#include <stdarg.h>
#include <util/logger.h>

typedef struct
{
    const unsigned char * data;
    uint32_t n;
} mp_bin_t;

/* use only when UTF8 is guaranteed */
typedef struct
{
    const char * data;
    uint32_t n;
} mp_str_t;

typedef struct
{
    const unsigned char * data;
    uint32_t n;
    int8_t tp;  /* must be last for alignment of the above */

    uint8_t  pad0_;
    uint16_t pad1_;
} mp_ext_t;

typedef union
{
    int64_t i64;
    uint64_t u64;
    double f64;
    mp_bin_t bin;
    mp_str_t str;
    _Bool bool_;
    size_t sz; /* used for both array and map */
    mp_ext_t ext;
} mp_via_t;

typedef enum
{
    MP_NEVER_USED=-3,
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
    MP_EXT
} mp_enum_t;

typedef struct
{
    const char * pt;
    const char * end;
} mp_unp_t;

typedef struct
{
    mp_enum_t tp;
    mp_via_t via;
} mp_obj_t;

static inline const char * __attribute__((unused))mp_type_str(mp_enum_t tp)
{
    switch (tp)
    {
    case MP_NEVER_USED:         return "marked as never used";
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
    case MP_EXT:                return "extension";
    }
    return "msgpack type unknown";
}

/* return 0 if successful
 *
 * reserve space at the start of `sbuffer` so we can later use the buffer
 * to make another type, for example add a header.
 *
 * nothing will be allocated if alloc and size are equal and in this case
 * the return value is always 0
 */
static inline int mp_sbuffer_alloc_init(
        msgpack_sbuffer * buffer,
        size_t alloc,
        size_t size)
{
    assert (alloc >= size);
    buffer->alloc = alloc;
    buffer->data = alloc == size ? NULL : malloc(alloc);
    buffer->size = size;
    return alloc != size && buffer->data == NULL;
}

static int __attribute__((unused))mp_pack_fmt(msgpack_packer * x, const char * fmt, ...)
{
    int rc, n;
    va_list args1, args2;
    char * body;

    va_start(args1, fmt);
    va_copy(args2, args1);
    n = vsnprintf(NULL, 0, fmt, args1);
    va_end(args1);

    if (n < 0 || msgpack_pack_str(x, n))
        return -1;

    body = malloc(n+1);
    if (!body)
        return -1;

    vsprintf(body, fmt, args2);
    va_end(args2);

    rc = n ? msgpack_pack_str_body(x, body, n) : 0;
    free(body);
    return rc;
}

static int __attribute__((unused))mp_pack_bool(msgpack_packer * x, _Bool b)
{
    return b ? msgpack_pack_true(x) : msgpack_pack_false(x);
}

static int __attribute__((unused))mp_pack_bin(msgpack_packer * x, const void * b, size_t n)
{
    return msgpack_pack_bin(x, n) || (n && msgpack_pack_bin_body(x, b, n));
}

static int __attribute__((unused))mp_pack_strn(msgpack_packer * x, const void * s, size_t n)
{
    return msgpack_pack_str(x, n) || (n && msgpack_pack_str_body(x, s, n));
}

#define mp_pack_str(x__, s__) \
    mp_pack_strn(x__, s__, strlen(s__))

#define mp_str_eq(o__, s__) (\
    (o__)->via.str.n == strlen(s__) && \
    memcmp(s__, (o__)->via.str.data, (o__)->via.str.n) == 0)

#define mp_strn_eq(o__, s__, n__) (\
    (o__)->via.str.n == n__ && \
    memcmp(s__, (o__)->via.str.data, (o__)->via.str.n) == 0)

#define mp_pack_append(pk__, s__, n__) \
    (*(pk__)->callback)((pk__)->data, (const char*)s__, n__)

static inline void mp_unp_init(mp_unp_t * up, const void * data, size_t n)
{
    up->pt = data;
    up->end = up->pt + n;
}

#define mp_read_8(tp__, addr__, up__, ret__) \
do { \
    if (up__->pt >= up__->end) \
        return (*(ret__) = MP_INCOMPLETE); \
    memcpy(addr__, up__->pt, sizeof(tp__)); \
    ++up__->pt; \
} while(0)

#define mp_read_skip_8(tp__, addr__, up__) \
do { mp_enum_t ret; mp_read_8(tp__, addr__, up__, &ret); } while(0)

#define mp_read_16(tp__, addr__, up__, ret__) \
do { \
    if (up__->pt + 1 >= up__->end) \
        return (*(ret__) = MP_INCOMPLETE); \
    _msgpack_load16(tp__, up__->pt, addr__); \
    up__->pt += 2; \
} while(0)

#define mp_read_skip_16(tp__, addr__, up__) \
do { mp_enum_t ret; mp_read_16(tp__, addr__, up__, &ret); } while(0)

#define mp_read_32(tp__, addr__, up__, ret__) \
do { \
    if (up__->pt + 3 >= up__->end) \
        return (*(ret__) = MP_INCOMPLETE); \
    _msgpack_load32(tp__, up__->pt, addr__); \
    up__->pt += 4; \
} while(0)

#define mp_read_skip_32(tp__, addr__, up__) \
do { mp_enum_t ret; mp_read_32(tp__, addr__, up__, &ret); } while(0)

#define mp_read_64(tp__, addr__, up__, ret__) \
do { \
    if (up__->pt + 7 >= up__->end) \
        return (*(ret__) = MP_INCOMPLETE); \
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
        return (o->tp = MP_INCOMPLETE); \
} while(0)

#define mp_read_ext_type(o__, up__) \
do { \
    if (up__->pt >= up__->end) \
        return (o->tp = MP_INCOMPLETE); \
    o__->via.ext.tp = *up__->pt; \
    ++up__->pt; \
} while(0)

static mp_enum_t __attribute__((unused))mp_next(mp_unp_t * up, mp_obj_t * o)
{
    unsigned char token;

    if (up->pt >= up->end)
        return (o->tp = MP_END);

    token = (unsigned char) *up->pt;
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
        return (o->tp = MP_NEVER_USED);
    case 0xc2:              /* false */
        o->via.bool_ = false;
        return (o->tp = MP_BOOL);
    case 0xc3:              /* true */
        o->via.bool_ = true;
        return (o->tp = MP_BOOL);
    case 0xc4:              /* bin 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up, &o->tp);
        o->via.str.n = u8;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc5:              /* bin 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.str.n = u16;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc6:              /* bin 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.str.n = u32;
        mp_read_obj_data(o, up);
        return (o->tp = MP_BIN);
    }
    case 0xc7:              /* ext 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up, &o->tp);
        o->via.ext.n = u8;
        mp_read_ext_type(o, up);
        mp_read_obj_data(o, up);
        return (o->tp = MP_EXT);
    }
    case 0xc8:              /* ext 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.ext.n = u16;
        mp_read_ext_type(o, up);
        mp_read_obj_data(o, up);
        return (o->tp = MP_EXT);
    }
    case 0xc9:              /* ext 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.ext.n = u32;
        mp_read_ext_type(o, up);
        mp_read_obj_data(o, up);
        return (o->tp = MP_EXT);
    }
    case 0xca:              /* float 32 */
    {
        union { float f; uint32_t u; } mem;
        mp_read_32(uint32_t, &mem.u, up, &o->tp);
        o->via.f64 = (double) mem.f;
        return (o->tp = MP_F64);
    }
    case 0xcb:              /* float 64 */
    {
        union { double d; uint64_t u; } mem;
        mp_read_64(uint64_t, &mem.u, up, &o->tp);
        o->via.f64 = mem.d;
        return (o->tp = MP_F64);
    }
    case 0xcc:              /* uint 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up, &o->tp);
        o->via.u64 = u8;
        return (o->tp = MP_U64);
    }
    case 0xcd:              /* uint 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.u64 = u16;
        return (o->tp = MP_U64);
    }
    case 0xce:              /* uint 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.u64 = u32;
        return (o->tp = MP_U64);
    }
    case 0xcf:              /* uint 64 */
    {
        uint64_t u64;
        mp_read_64(uint64_t, &u64, up, &o->tp);
        o->via.u64 = u64;
        return (o->tp = MP_U64);
    }
    case 0xd0:              /* int 8 */
    {
        int8_t i8;
        mp_read_8(int8_t, &i8, up, &o->tp);
        o->via.i64 = i8;
        return (o->tp = MP_I64);
    }
    case 0xd1:              /* int 16 */
    {
        int16_t i16;
        mp_read_16(int16_t, &i16, up, &o->tp);
        o->via.i64 = i16;
        return (o->tp = MP_I64);
    }
    case 0xd2:              /* int 32 */
    {
        int32_t i32;
        mp_read_32(int32_t, &i32, up, &o->tp);
        o->via.i64 = i32;
        return (o->tp = MP_I64);
    }
    case 0xd3:              /* int 64 */
    {
        int64_t i64;
        mp_read_64(int64_t, &i64, up, &o->tp);
        o->via.i64 = i64;
        return (o->tp = MP_I64);
    }
    case 0xd4:              /* fixext 1 */
    case 0xd5:              /* fixext 2 */
    case 0xd6:              /* fixext 4 */
    case 0xd7:              /* fixext 8 */
    case 0xd8:              /* fixext 16 */
    {
        o->via.ext.n = 1 << (token - 0xd4);
        mp_read_ext_type(o, up);
        mp_read_obj_data(o, up);
        return (o->tp = MP_EXT);
    }
    case 0xd9:              /* str 8 */
    {
        uint8_t u8;
        mp_read_8(uint8_t, &u8, up, &o->tp);
        o->via.str.n = u8;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xda:              /* str 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.str.n = u16;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdb:              /* str 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.str.n = u32;
        mp_read_obj_data(o, up);
        return (o->tp = MP_STR);
    }
    case 0xdc:              /* array 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.sz = u16;
        return (o->tp = MP_ARR);
    }
    case 0xdd:              /* array 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.sz = u32;
        return (o->tp = MP_ARR);
    }
    case 0xde:              /* map 16 */
    {
        uint16_t u16;
        mp_read_16(uint16_t, &u16, up, &o->tp);
        o->via.sz = u16;
        return (o->tp = MP_MAP);
    }
    case 0xdf:              /* map 32 */
    {
        uint32_t u32;
        mp_read_32(uint32_t, &u32, up, &o->tp);
        o->via.sz = u32;
        return (o->tp = MP_MAP);
    }
    case 0xe0 ... 0xff:     /* fixed negative */
        o->via.i64 = (char) token;
        return (o->tp = MP_I64);
    }

    return (o->tp = MP_ERR);
}

static mp_enum_t __attribute__((unused))mp_peek(mp_unp_t * up, mp_obj_t * o)
{
    const char * keep = up->pt;
    mp_enum_t tp = mp_next(up, o);
    up->pt = keep;
    return tp;
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
        return MP_NEVER_USED;
    case 0xc2:              /* false */
    case 0xc3:              /* true */
        return MP_BOOL;
    case 0xc4:              /* bin 8 */
    {
        uint8_t u8;
        mp_read_skip_8(uint8_t, &u8, up);
        mp_skip_chars(u8, up);
        return MP_BIN;
    }
    case 0xc5:              /* bin 16 */
    {
        uint16_t u16;
        mp_read_skip_16(uint16_t, &u16, up);
        mp_skip_chars(u16, up);
        return MP_BIN;
    }
    case 0xc6:              /* bin 32 */
    {
        uint32_t u32;
        mp_read_skip_32(uint32_t, &u32, up);
        mp_skip_chars(u32, up);
        return MP_BIN;
    }
    case 0xc7:              /* ext 8 */
    {
        uint8_t u8;
        mp_read_skip_8(uint8_t, &u8, up);
        mp_skip_chars(u8+1, up);
        return MP_EXT;
    }
    case 0xc8:              /* ext 16 */
    {
        uint16_t u16;
        mp_read_skip_16(uint16_t, &u16, up);
        mp_skip_chars(u16+1, up);
        return MP_EXT;
    }
    case 0xc9:              /* ext 32 */
    {
        uint32_t u32;
        mp_read_skip_32(uint32_t, &u32, up);
        mp_skip_chars(u32+1, up);
        return MP_EXT;
    }
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
    {
        uint8_t n = 1 << (token - 0xd4);
        mp_skip_chars(n+1, up);
        return MP_EXT;
    }
    case 0xd9:              /* str 8 */
    {
        uint8_t u8;
        mp_read_skip_8(uint8_t, &u8, up);
        mp_skip_chars(u8, up);
        return MP_STR;
    }
    case 0xda:              /* str 16 */
    {
        uint16_t u16;
        mp_read_skip_16(uint16_t, &u16, up);
        mp_skip_chars(u16, up);
        return MP_STR;
    }
    case 0xdb:              /* str 32 */
    {
        uint32_t u32;
        mp_read_skip_32(uint32_t, &u32, up);
        mp_skip_chars(u32, up);
        return MP_STR;
    }
    case 0xdc:              /* array 16 */
    {
        uint16_t u16;
        mp_read_skip_16(uint16_t, &u16, up);
        mp_skip_n(u16, up);
        return MP_ARR;
    }
    case 0xdd:              /* array 32 */
    {
        uint32_t u32;
        mp_read_skip_32(uint32_t, &u32, up);
        mp_skip_n(u32, up);
        return MP_ARR;
    }
    case 0xde:              /* map 16 */
    {
        uint16_t u16;
        mp_read_skip_16(uint16_t, &u16, up);
        mp_skip_n(u16*2, up);
        return MP_MAP;
    }
    case 0xdf:              /* map 32 */
    {
        uint32_t u32;
        mp_read_skip_32(uint32_t, &u32, up);
        mp_skip_n(u32*2, up);
        return MP_MAP;
    }
    case 0xe0 ... 0xff:     /* fixed negative */
        return MP_I64;
    }

    return MP_ERR;
}

static void __attribute__((unused))mp_print_up(FILE * out, mp_unp_t * up)
{
    mp_obj_t obj;
    switch (mp_next(up, &obj))
    {
    case MP_NEVER_USED:
        fputs("MP_NEVER_USED", out);
        return;
    case MP_INCOMPLETE:
        fputs("MP_INCOMPLETE", out);
        return;
    case MP_ERR:
        fputs("MP_ERR", out);
        return;
    case MP_END:
        return;
    case MP_I64:
        fprintf(out, "%"PRIi64, obj.via.i64);
        return;
    case MP_U64:
        fprintf(out, "%"PRIu64, obj.via.u64);
        return;
    case MP_F64:
        fprintf(out, "%f", obj.via.f64);
        return;
    case MP_BIN:
        fputs("<BINARY>", out);
        return;
    case MP_STR:
        putc('"', out);
        for (const char * c = obj.via.str.data,
                        * e = obj.via.str.data + obj.via.str.n;
             c < e;
             ++c)
        {
            if (*c == '"')
                putc('\\', out);
            putc(*c, out);
        }
        putc('"', out);
        return;
    case MP_BOOL:
        if (obj.via.bool_)
            fputs("true", out);
        else
            fputs("false", out);
        return;
    case MP_NIL:
        fputs("nil", out);
        return;
    case MP_ARR:
    {
        size_t i = 0, m = obj.via.sz;
        putc('[', out);
        while (i < m)
        {
            i++;
            mp_print_up(out, up);
            if (i == m)
                break;
            putc(',', out);
        }
        putc(']', out);
        return;
    }
    case MP_MAP:
    {
        size_t i = 0, m = obj.via.sz;
        putc('{', out);
        while (i < m)
        {
            i++;
            mp_print_up(out, up);
            putc(':', out);
            mp_print_up(out, up);
            if (i == m)
                break;
            putc(',', out);
        }
        putc('}', out);
        return;
    }
    case MP_EXT:
        fprintf(out, "<EXT TYPE:%d>", obj.via.ext.tp);
        return;
    }
}

static void __attribute__((unused))mp_print(FILE * out, const void * data, size_t n)
{
    mp_unp_t up;
    mp_unp_init(&up, data, n);
    mp_print_up(out, &up);
}

static _Bool __attribute__((unused))mp_is_valid(const void * data, size_t n)
{
    mp_unp_t up;
    mp_unp_init(&up, data, n);
    return mp_skip(&up) > MP_END && mp_skip(&up) == MP_END;
}

static inline int mp_cast_u64(mp_obj_t * o)
{
    if (o->tp == MP_U64)
        return 0;
    if (o->tp == MP_I64)
    {
        o->via.u64 = (uint64_t) o->via.i64;
        return 0;
    }
    return -1;
}

static inline int mp_cast_i64(mp_obj_t * o)
{
    if (o->tp == MP_I64)
        return 0;
    if (o->tp == MP_U64)
    {
        if (o->via.u64 > LLONG_MAX)
            return -1;
        o->via.i64 = (int64_t) o->via.u64;
        return 0;
    }
    return -1;
}

static inline void * mp_strdup(mp_obj_t * o)
{
    assert (o->tp == MP_STR || o->tp == MP_BIN);
    return strndup(o->via.str.data, o->via.str.n);
}

#endif  /* MPACK_H_ */
