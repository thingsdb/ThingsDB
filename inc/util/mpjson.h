/*
 * util/mpjson.h
 *
 * TESTED with:
 * - msgpack version: 3.2.0
 * - yajl version: 2.1.0
 */

#ifndef MPJSON_H_
#define MPJSON_H_

#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <inttypes.h>
#include <util/mpack.h>

enum
{
    /* flags map to the API flags */
    MPJSON_FLAG_BEAUTIFY        =1<<3,
    MPJSON_FLAG_VALIDATE_UTF8   =1<<4,
};

typedef struct
{
    msgpack_packer pk;
    size_t deep;
    size_t buf_n[YAJL_MAX_DEPTH];
    uint32_t count[YAJL_MAX_DEPTH];
} mpjson_convert_t;

static yajl_gen_status mp__to_json(yajl_gen g, mp_unp_t * up)
{
    mp_obj_t obj;
    switch (mp_next(up, &obj))
    {
    case MP_NEVER_USED:
        return yajl_gen_in_error_state;
    case MP_INCOMPLETE:
        return yajl_gen_in_error_state;
    case MP_ERR:
        return yajl_gen_in_error_state;
    case MP_END:
        return yajl_gen_status_ok;
    case MP_I64:
        return yajl_gen_integer(g, obj.via.i64);
    case MP_U64:
        if (obj.via.u64 > INT64_MAX)
        {
            char buf[21];
            int len = sprintf(buf, "%"PRIu64, obj.via.u64);
            return len > 0
                    ? yajl_gen_number(g, buf, (size_t) len)
                    : yajl_gen_in_error_state;
        }
        return yajl_gen_integer(g, (int64_t) obj.via.u64);
    case MP_F64:
        return yajl_gen_double(g, obj.via.f64);
    case MP_BIN:
        return yajl_gen_invalid_string;
    case MP_STR:
        return yajl_gen_string(
                g, (unsigned char *) obj.via.str.data, obj.via.str.n);
    case MP_BOOL:
        return yajl_gen_bool(g, obj.via.bool_);
    case MP_NIL:
        return yajl_gen_null(g);
    case MP_ARR:
    {
        size_t i = obj.via.sz;
        yajl_gen_status stat;
        if ((stat = yajl_gen_array_open(g)) != yajl_gen_status_ok)
            return stat;

        while (i--)
            if ((stat = mp__to_json(g, up)) != yajl_gen_status_ok)
                return stat;

        return yajl_gen_array_close(g);
    }
    case MP_MAP:
    {
        size_t i = obj.via.sz;
        yajl_gen_status stat;
        if ((stat = yajl_gen_map_open(g)) != yajl_gen_status_ok)
            return stat;

        while (i--)
        {
            if ((stat = mp__to_json(g, up)) != yajl_gen_status_ok ||
                (stat = mp__to_json(g, up)) != yajl_gen_status_ok)
                return stat;
        }

        return yajl_gen_map_close(g);
    }
    case MP_EXT:
        return yajl_gen_in_error_state;
    }
    return yajl_gen_in_error_state;
}

static yajl_gen_status __attribute__((unused))mpjson_mp_to_json(
        const void * src,
        size_t src_n,
        unsigned char ** dst,
        size_t * dst_n,
        int flags)
{
    mp_unp_t up;
    yajl_gen g;
    yajl_gen_status stat;

    mp_unp_init(&up, src, src_n);

    g = yajl_gen_alloc(NULL);
    if (!g)
        return yajl_gen_in_error_state;

    yajl_gen_config(g, yajl_gen_beautify, flags & MPJSON_FLAG_BEAUTIFY);
    yajl_gen_config(g, yajl_gen_validate_utf8, flags & MPJSON_FLAG_VALIDATE_UTF8);

    if ((stat = mp__to_json(g, &up)) == yajl_gen_status_ok)
    {
        const unsigned char * tmp;
        if ((stat = yajl_gen_get_buf(g, &tmp, dst_n)) == yajl_gen_status_ok)
        {
            *dst = malloc(*dst_n);
            if (*dst)
                memcpy(*dst, tmp, *dst_n);
            else
                stat = yajl_gen_in_error_state;
        }
    }

    yajl_gen_free(g);
    return stat;
}

static int reformat_null(void * ctx)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    ++c->count[c->deep];
    return 0 == msgpack_pack_nil(&c->pk);
}

static int reformat_boolean(void * ctx, int boolean)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    ++c->count[c->deep];
    return 0 == mp_pack_bool(&c->pk, boolean);
}

static int reformat_integer(void * ctx, long long i)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    ++c->count[c->deep];
    return 0 == msgpack_pack_int64(&c->pk, i);
}

static int reformat_double(void * ctx, double d)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    ++c->count[c->deep];
    return 0 == msgpack_pack_double(&c->pk, d);
}

static int reformat_string(void * ctx, const unsigned char * s, size_t n)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    ++c->count[c->deep];
    return 0 == mp_pack_strn(&c->pk, s, n);
}

static int reformat_map_key(void * ctx, const unsigned char * s, size_t n)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    return 0 == mp_pack_strn(&c->pk, s, n);
}

static int reformat_start_map(void * ctx)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    msgpack_sbuffer * buffer = c->pk.data;

    ++c->count[c->deep];
    if (++c->deep > YAJL_MAX_DEPTH)
        return 0;  /* error */

    c->count[c->deep] = 0;
    c->buf_n[c->deep] = buffer->size+1;

    return 0 == msgpack_pack_map(&c->pk, 0x10000UL);
}


static int reformat_end_map(void * ctx)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    msgpack_sbuffer * buffer = c->pk.data;

    _msgpack_store32(buffer->data + c->buf_n[c->deep], c->count[c->deep]);
    --c->deep;

    return 1;  /* success */
}

static int reformat_start_array(void * ctx)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    msgpack_sbuffer * buffer = c->pk.data;

    ++c->count[c->deep];
    if (++c->deep > YAJL_MAX_DEPTH)
        return 0;  /* error */

    c->count[c->deep] = 0;
    c->buf_n[c->deep] = buffer->size+1;

    return 0 == msgpack_pack_array(&c->pk, 0x10000L);
}

static int reformat_end_array(void * ctx)
{
    mpjson_convert_t * c = (mpjson_convert_t *) ctx;
    msgpack_sbuffer * buffer = c->pk.data;

    _msgpack_store32(buffer->data + c->buf_n[c->deep], c->count[c->deep]);
    --c->deep;

    return 1;  /* success */
}

static yajl_callbacks mpjson__callbacks = {
    reformat_null,
    reformat_boolean,
    reformat_integer,
    reformat_double,
    NULL,
    reformat_string,
    reformat_start_map,
    reformat_map_key,
    reformat_end_map,
    reformat_start_array,
    reformat_end_array
};

static void take_buffer(
        msgpack_sbuffer * buffer,
        char ** dst,
        size_t * dst_n)
{
    *dst = buffer->data;
    *dst_n = buffer->size;
    memset(buffer, 0, sizeof(msgpack_sbuffer));
}

static yajl_status __attribute__((unused))mpjson_json_to_mp(
        const void * src,
        size_t src_n,
        char ** dst,
        size_t * dst_n)
{
    yajl_handle hand;
    msgpack_sbuffer buffer;
    yajl_status stat = yajl_status_error;
    mpjson_convert_t ctx = {0};

    if (mp_sbuffer_alloc_init(&buffer, src_n, 0))
        return stat;

    msgpack_packer_init(&ctx.pk, &buffer, msgpack_sbuffer_write);

    hand = yajl_alloc(&mpjson__callbacks, NULL, &ctx);
    if (!hand)
        goto fail;

    stat = yajl_parse(hand, src, src_n);
    if (stat == yajl_status_ok)
        take_buffer(&buffer, dst, dst_n);

    yajl_free(hand);
fail:
    msgpack_sbuffer_destroy(&buffer);
    return stat;
}

#endif  /* MPJSON_H_ */
