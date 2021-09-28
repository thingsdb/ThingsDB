/*
 * ti/mod/mod.c
 */


#include <ti/mod/manifest.t.h>
#include <stdio.h>
#include <stdargs.h>

typedef enum
{
    MOD__ROOT,
    MOD__ROOT_MAP,
    MOD__MAIN,
    MOD__MAIN_MAP,          /* main may be a architecture map */
} mod__ctx_mode_t;

typedef struct
{
    mod__ctx_mode_t mode;
    ti_mod_manifest_t * manifest;
    char * source_err;         /* pointer to module->source_err which is
                                  limited by (TI_MODULE_MAX_ERR */
} mod__ctx_t;

static inline int mod__set_err(mod__ctx_t * ctx, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(ctx->source_err, TI_MODULE_MAX_ERR, fmt, args);
    va_end(args);
    return 0;  /* failed, yajl_status_client_canceled */
}

static inline int mod__set_mode(mod__ctx_t * ctx, mod__ctx_mode_t mode)
{
    ctx->mode = mode;
    return 1;  /* success */
}

static inline _Bool mod__key_equals(
        const void * a, size_t an,
        const void * b, size_t bn)
{
    return an == bn && memcmp(a, b, 4) == 0;
}

static int mod__json_null(void * data)
{
    mod__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MOD__ROOT:
        return mod__set_err(ctx, "expecting a map in "TI_MANIFEST", not null");

    }
    return mod__set_err(ctx, "unexpected error in "TI_MANIFEST);
}

static int mod__json_string(void * data, const unsigned char * s, size_t n)
{
    mod__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MOD__ROOT:
        return mod__set_err(ctx, TI_MOD_MANIFEST_EXPECT_ROOT_MAP);
    case MOD__MAIN:
        if (!ctx->manifest->main)
            ctx->manifest->main = strndup(s, n);
        return mod__set_mode(ctx, MOD__ROOT_MAP);
    }
    return mod__set_err(ctx, TI_MOD_MANIFEST_UNEXPECTED_ERROR);
}

static int mod__json_start_map(void * data)
{
    mod__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MOD__ROOT:
        return mod__set_mode(ctx, MOD__ROOT_MAP);
    case MOD__MAIN:
        return mod__set_mode(ctx, MOD__MAIN_MAP);
    }
    return mod__set_err(ctx, TI_MOD_MANIFEST_UNEXPECTED_ERROR);
}

static int mod__json_map_key(void * data, const unsigned char * s, size_t n)
{
    mod__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MOD__ROOT_MAP:
        if (mod__key_equals(s, n, "main", 4))
            return mod__set_mode(ctx, MOD__MAIN);

    case MOD__MAIN_MAP:

    }
    return mod__set_err(ctx, TI_MOD_MANIFEST_UNEXPECTED_ERROR);
}


static yajl_callbacks mod__callbacks = {
    mod__json_null,
    mod__json_boolean,
    mod__json_integer,
    mod__json_double,
    NULL,
    mod__json_string,
    mod__json_start_map,
    mod__json_map_key,
    mod__json_end_map,
    mod__json_start_array,
    mod__json_end_array
};



yajl_status gh__read_module_json(
        const void * src,
        size_t src_n,
        char ** dst,
        size_t * dst_n)
{
    yajl_handle hand;
    yajl_status stat = yajl_status_error;
    mod__ctx_t ctx = {0};

    hand = yajl_alloc(&mod__callbacks, NULL, ctx);
    if (!hand)
        goto fail1;

    stat = yajl_parse(hand, src, src_n);
    if (stat == yajl_status_ok)
        take_buffer(&buffer, dst, dst_n);

    yajl_free(hand);
fail1:
    msgpack_sbuffer_destroy(&buffer);
fail0:
    free(c);
    return stat;
}
