/*
 * ti/mod/manifest.c
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <ti/item.t.h>
#include <ti/mod/expose.h>
#include <ti/mod/expose.t.h>
#include <ti/mod/manifest.h>
#include <ti/mod/manifest.t.h>
#include <ti/module.t.h>
#include <ti/names.h>
#include <ti/nil.h>
#include <ti/raw.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/vbool.h>
#include <ti/version.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <tiinc.h>
#include <util/logger.h>
#include <util/mpjson.h>
#include <util/osarch.h>
#include <util/strx.h>
#include <yajl/yajl_parse.h>

typedef enum
{
    MF__ROOT,
    MF__ROOT_MAP,
    MF__MAIN,
    MF__MAIN_MAP,          /* main may be a architecture map */
    MF__MAIN_OSARCH,
    MF__MAIN_NO_OSARCH,
    MF__VERSION,
    MF__DOC,
    MF__DEFAULTS,
    MF__DEFAULTS_MAP,
    MF__DEFAULTS_DEEP,
    MF__DEFAULTS_LOAD,
    MF__DEFAULTS_ITEM,
    MF__DEFAULTS_PACK,
    MF__INCLUDES,
    MF__INCLUDES_ARR,
    MF__INCLUDES_MAP,
    MF__INCLUDES_OSARCH,
    MF__INCLUDES_NO_OSARCH,
    MF__EXPOSES,
    MF__EXPOSES_MAP,
    MF__X,
    MF__X_MAP,
    MF__X_DOC,
    MF__X_DEFAULTS,
    MF__X_DEFAULTS_MAP,
    MF__X_DEFAULTS_DEEP,
    MF__X_DEFAULTS_LOAD,
    MF__X_DEFAULTS_ITEM,
    MF__X_DEFAULTS_PACK,
} manifest__ctx_mode_t;

typedef struct
{
    manifest__ctx_mode_t mode;
    ti_mod_manifest_t * manifest;
    char * source_err;          /* pointer to module->source_err which is
                                   limited by (TI_MODULE_MAX_ERR */
    void * data;                /* pointer to anything */
} manifest__ctx_t;

typedef struct
{
    mpjson_convert_t ctx;       /* pointer to unpack context */
    msgpack_sbuffer buffer;
    ti_item_t * item;
} manifest__up_t;

static inline int manifest__set_err(
        manifest__ctx_t * ctx,
        const char * fmt,
        ...)
{
    va_list args;
    va_start(args, fmt);
    (void) vsnprintf(ctx->source_err, TI_MODULE_MAX_ERR, fmt, args);
    va_end(args);
    return 0;  /* failed, yajl_status_client_canceled */
}

static inline int manifest__set_mode(
        manifest__ctx_t * ctx,
        manifest__ctx_mode_t mode)
{
    ctx->mode = mode;
    return 1;  /* success */
}

static inline _Bool manifest__key_equals(
        const void * a, size_t an,
        const void * b, size_t bn)
{
    return an == bn && memcmp(a, b, an) == 0;
}

static int manifest__json_null(void * data)
{
    LOGC("NULL");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not null");
    case MF__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not null");
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not null");
    case MF__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not null");
    case MF__DOC:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not null");
    case MF__DEFAULTS_DEEP:
        return manifest__set_err(
                ctx,
                "expecting a `deep` value between 0 and %d, not null",
                TI_MAX_DEEP_HINT);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not null");
    case MF__DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_nil_get();
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    case MF__DEFAULTS_PACK:
        return reformat_null(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not null");
    case MF__INCLUDES_ARR:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `include` item, not null");
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not null");
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not null");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not null");
    case MF__X_DOC:
        return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not null");
    case MF__X_DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_nil_get();
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_PACK:
        return reformat_null(&((manifest__up_t *) ctx->data)->ctx);

    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_boolean(void * data, int boolean)
{
    LOGC("BOOLEAN");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a boolean");
    case MF__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a boolean");
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a boolean");
    case MF__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not a boolean");
    case MF__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a boolean");
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not a boolean");
    case MF__DEFAULTS_DEEP:
        return manifest__set_err(
                ctx,
                "expecting a `deep` value between 0 and %d, not a boolean",
                TI_MAX_DEEP_HINT);
    case MF__DEFAULTS_LOAD:
        if (!ctx->manifest->load)
        {
            ctx->manifest->load = malloc(sizeof(_Bool));
            if (ctx->manifest->load)
                *ctx->manifest->load = boolean;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_vbool_get(boolean);
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    case MF__DEFAULTS_PACK:
        return reformat_boolean(&((manifest__up_t *) ctx->data)->ctx, boolean);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not a boolean");
    case MF__INCLUDES_ARR:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `include` item, not a boolean");
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not a boolean");
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not a boolean");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not a boolean");
    case MF__X_DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc`, not a boolean");
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not a boolean");
    case MF__X_DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_vbool_get(boolean);
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_PACK:
        return reformat_boolean(&((manifest__up_t *) ctx->data)->ctx, boolean);

    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_integer(void * data, long long i)
{
    LOGC("INTEGER");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a number");
    case MF__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a number");
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    case MF__VERSION:
        if (!ctx->manifest->version)
            (void) asprintf(&ctx->manifest->version, "%lld", i);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a number");
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not a number");
    case MF__DEFAULTS_DEEP:
        if (i < 0 || i > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d, not %lld",
                    TI_MAX_DEEP_HINT, i);
        if (!ctx->manifest->deep)
        {
            ctx->manifest->deep = malloc(sizeof(uint8_t));
            if (ctx->manifest->deep)
                *ctx->manifest->deep = (uint8_t) i;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not a number");
    case MF__DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_vint_create(i);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    case MF__DEFAULTS_PACK:
        return reformat_integer(&((manifest__up_t *) ctx->data)->ctx, i);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not a number");
    case MF__INCLUDES_ARR:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `include` item, not a number");
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not a number");
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not a number");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not a number");
    case MF__X_DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc`, not a number");
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not a number");
    case MF__X_DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_vint_create(i);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_PACK:
        return reformat_integer(&((manifest__up_t *) ctx->data)->ctx, i);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_double(void * data, double d)
{
    LOGC("DOUBLE");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a number");
    case MF__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a number");
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    case MF__VERSION:
        if (!ctx->manifest->version)
            (void) asprintf(&ctx->manifest->version, "%g", d);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a number");
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not a number");
    case MF__DEFAULTS_DEEP:
        if (d < 0 || d > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d, not %f",
                    TI_MAX_DEEP_HINT, d);
        if (!ctx->manifest->deep)
        {
            ctx->manifest->deep = malloc(sizeof(uint8_t));
            if (ctx->manifest->deep)
                *ctx->manifest->deep = (uint8_t) d;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not a number");
    case MF__DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *)  ti_vfloat_create(d);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    case MF__DEFAULTS_PACK:
        return reformat_double(&((manifest__up_t *) ctx->data)->ctx, d);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not a number");
    case MF__INCLUDES_ARR:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `include` item, not a number");
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not a number");
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not a number");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not a number");
    case MF__X_DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc`, not a number");
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not a number");
    case MF__X_DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *)  ti_vfloat_create(d);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_PACK:
        return reformat_double(&((manifest__up_t *) ctx->data)->ctx, d);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_string(
        void * data,
        const unsigned char * s,
        size_t n)
{
    LOGC("STRING");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a string");
    case MF__MAIN:
        if (!ctx->manifest->main)
            ctx->manifest->main = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__MAIN_OSARCH:
        if (!ctx->manifest->main)
            ctx->manifest->main = strndup((const char *) s, n);
        /* fall through */
    case MF__MAIN_NO_OSARCH:
        return manifest__set_mode(ctx, MF__MAIN_MAP);
    case MF__VERSION:
        if (!ctx->manifest->version)
            ctx->manifest->version = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DOC:
        if (!ctx->manifest->doc)
            ctx->manifest->doc = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not a string");
    case MF__DEFAULTS_DEEP:
        return manifest__set_err(
                ctx,
                "expecting a `deep` value between 0 and %d, not a string",
                TI_MAX_DEEP_HINT);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not a string");
    case MF__DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_str_create((const char *) s, n);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    case MF__DEFAULTS_PACK:
        return reformat_string(&((manifest__up_t *) ctx->data)->ctx, s, n);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not a string");
    case MF__INCLUDES_ARR:
    {
        char * str = strndup((const char *) s, n);
        return str && vec_push_create(&ctx->manifest->includes, str) == 0;
    }
    case MF__INCLUDES_OSARCH:
    {
        char * str = strndup((const char *) s, n);
        return str && vec_push_create(&ctx->manifest->includes, str) == 0;
    }
    /* fall through */
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_mode(ctx, MF__INCLUDES_MAP);
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not a string");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not a string");
    case MF__X_DOC:
    {
        ti_mod_expose_t * expose = ctx->data;
        if (!expose->doc)
            expose->doc = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MF__X_MAP);
    }
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not a string");
    case MF__X_DEFAULTS_ITEM:
    {
        ti_item_t * item = ctx->data;
        item->val = (ti_val_t *) ti_str_create((const char *) s, n);
        if (!item->val)
            return manifest__set_err(ctx, "allocation error");
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_PACK:
        return reformat_string(&((manifest__up_t *) ctx->data)->ctx, s, n);

    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_start_map(void * data)
{
    LOGC("STAT MAP");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__MAIN:
        return manifest__set_mode(ctx, MF__MAIN_MAP);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a map");
    case MF__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not a map");
    case MF__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a map");
    case MF__DEFAULTS:
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_DEEP:
        return manifest__set_err(
                ctx,
                "expecting a `deep` value between 0 and %d, not a map",
                TI_MAX_DEEP_HINT);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not a map");
    case MF__DEFAULTS_ITEM:
    {
        manifest__up_t * up = calloc(1, sizeof(manifest__up_t));
        if (!up || mp_sbuffer_alloc_init(&up->buffer, 8192, sizeof(ti_raw_t)))
        {
            free(up);
            return manifest__set_err(ctx, "allocation error");
        }

        msgpack_packer_init(&up->ctx.pk, &up->buffer, msgpack_sbuffer_write);

        up->item = ctx->data;
        ctx->data = up;

        (void) manifest__set_mode(ctx, MF__DEFAULTS_PACK);
    }
    /*fall through */
    case MF__DEFAULTS_PACK:
        return reformat_start_map(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:
        return manifest__set_err(
                ctx,
                "expecting an array with `includes` "
                "in "TI_MANIFEST", not a map");
    case MF__INCLUDES_ARR:
        return manifest__set_mode(ctx, MF__INCLUDES_MAP);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not a map");
    case MF__EXPOSES:
        return manifest__set_mode(ctx, MF__EXPOSES_MAP);
    case MF__X:
        return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc`, not a map");
    case MF__X_DEFAULTS:
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    case MF__X_DEFAULTS_ITEM:
    {
        manifest__up_t * up = calloc(1, sizeof(manifest__up_t));
        if (!up || mp_sbuffer_alloc_init(&up->buffer, 8192, sizeof(ti_raw_t)))
        {
            free(up);
            return manifest__set_err(ctx, "allocation error");
        }

        msgpack_packer_init(&up->ctx.pk, &up->buffer, msgpack_sbuffer_write);

        up->item = ctx->data;
        ctx->data = up;

        (void) manifest__set_mode(ctx, MF__X_DEFAULTS_PACK);
    }
    /*fall through */
    case MF__X_DEFAULTS_PACK:
        return reformat_start_map(&((manifest__up_t *) ctx->data)->ctx);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_map_key(
        void * data,
        const unsigned char * s,
        size_t n)
{
    LOGC("MAP KEY");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT_MAP:
        if (manifest__key_equals(s, n, "main", 4))
            return manifest__set_mode(ctx, MF__MAIN);
        if (manifest__key_equals(s, n, "version", 7))
            return manifest__set_mode(ctx, MF__VERSION);
        if (manifest__key_equals(s, n, "doc", 3))
            return manifest__set_mode(ctx, MF__DOC);
        if (manifest__key_equals(s, n, "defaults", 8))
            return manifest__set_mode(ctx, MF__DEFAULTS);
        if (manifest__key_equals(s, n, "includes", 8))
            return manifest__set_mode(ctx, MF__INCLUDES);
        if (manifest__key_equals(s, n, "exposes", 7))
            return manifest__set_mode(ctx, MF__EXPOSES);
        return manifest__set_err(
                ctx, "unsupported key `%.*s` in "TI_MANIFEST,
                n, s);
    case MF__MAIN_MAP:
    {
        const char * osarch = osarch_get();
        size_t sz = strlen(osarch);
        if (manifest__key_equals(s, n, osarch, sz))
            return manifest__set_mode(ctx, MF__MAIN_OSARCH);
        return manifest__set_mode(ctx, MF__MAIN_NO_OSARCH);
    }
    case MF__DEFAULTS_MAP:
        if (manifest__key_equals(s, n, "deep", 4))
            return manifest__set_mode(ctx, MF__DEFAULTS_DEEP);
        if (manifest__key_equals(s, n, "load", 4))
            return manifest__set_mode(ctx, MF__DEFAULTS_LOAD);
        {
            const char * str = (const char *) s;
            ti_item_t * item = malloc(sizeof(ti_item_t));
            if (!item)
                return manifest__set_err(ctx, "allocation error");

            item->val = NULL;
            item->key = ti_name_is_valid_strn(str, n)
                    ? (ti_raw_t *) ti_names_get(str, n)
                    : ti_str_create(str, n);

            if (!item->key || vec_push_create(&ctx->manifest->defaults, item))
            {
                ti_val_drop((ti_val_t *) item->key);
                free(item);
                return manifest__set_err(ctx, "allocation error");
            }

            ctx->data = item;
            return manifest__set_mode(ctx, MF__DEFAULTS_ITEM);
        }
    case MF__DEFAULTS_PACK:
        return reformat_map_key(&((manifest__up_t *) ctx->data)->ctx, s, n);
    case MF__INCLUDES_MAP:
    {
        const char * osarch = osarch_get();
        size_t sz = strlen(osarch);
        if (manifest__key_equals(s, n, osarch, sz))
            return manifest__set_mode(ctx, MF__INCLUDES_OSARCH);
        return manifest__set_mode(ctx, MF__INCLUDES_NO_OSARCH);
    }
    case MF__EXPOSES_MAP:
    {
        ti_mod_expose_t * expose = ti_expose_create();
        if (!expose ||
            smap_addn(ctx->manifest->exposes, (const char *) s, n, expose))
            return manifest__set_err(
                    ctx,
                    "double expose function in "TI_MANIFEST);
        ctx->data = expose;
        return manifest__set_mode(ctx, MF__X);
    }
    case MF__ROOT_MAP:
        if (manifest__key_equals(s, n, "doc", 3))
            return manifest__set_mode(ctx, MF__X_DOC);
        if (manifest__key_equals(s, n, "defaults", 8))
            return manifest__set_mode(ctx, MF__X_DEFAULTS);
        if (manifest__key_equals(s, n, "mapping", 7))
            return manifest__set_mode(ctx, MF__X_DEFAULTS);
        return manifest__set_err(
                ctx, "unsupported key `%.*s` in "TI_MANIFEST,
                n, s);
    case MF__X_DEFAULTS_MAP:
        if (manifest__key_equals(s, n, "deep", 4))
            return manifest__set_mode(ctx, MF__X_DEFAULTS_DEEP);
        if (manifest__key_equals(s, n, "load", 4))
            return manifest__set_mode(ctx, MF__X_DEFAULTS_LOAD);
        {
            ti_mod_expose_t * expose = ctx->data;
            const char * str = (const char *) s;
            ti_item_t * item = malloc(sizeof(ti_item_t));
            if (!item)
                return manifest__set_err(ctx, "allocation error");

            item->val = NULL;
            item->key = ti_name_is_valid_strn(str, n)
                    ? (ti_raw_t *) ti_names_get(str, n)
                    : ti_str_create(str, n);

            if (!item->key || vec_push_create(&expose->defaults, item))
            {
                ti_val_drop((ti_val_t *) item->key);
                free(item);
                return manifest__set_err(ctx, "allocation error");
            }

            return manifest__set_mode(ctx, MF__X_DEFAULTS_ITEM);
        }
    case MF__X_DEFAULTS_PACK:
        return reformat_map_key(&((manifest__up_t *) ctx->data)->ctx, s, n);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_end_map(void * data)
{
    LOGC("END MAP");
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT_MAP:
        return manifest__set_mode(ctx, MF__ROOT);
    case MF__MAIN_MAP:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS_MAP:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS_PACK:
    {
        manifest__up_t * up = ctx->data;
        int ok = reformat_end_map(&up->ctx);
        if (ok && up->ctx.deep == 0)
        {
            ti_item_t * item = ctx->data;
            ti_raw_t * raw;
            size_t dst_n;

            take_buffer(up->ctx.pk.data, (char **) &raw, &dst_n);
            ti_raw_init(raw, TI_VAL_MPDATA, dst_n);

            item->val = (ti_val_t *) raw;
            ctx->data = up->item;
            free(up);
            return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
        }
        return ok;
    }
    case MF__INCLUDES_MAP:
        return manifest__set_mode(ctx, MF__INCLUDES_ARR);
    case MF__EXPOSES_MAP:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__X_MAP:
        return manifest__set_mode(ctx, MF__EXPOSES_MAP);
    case MF__X_DEFAULTS_PACK:
    {
        manifest__up_t * up = ctx->data;
        int ok = reformat_end_map(&up->ctx);
        if (ok && up->ctx.deep == 0)
        {
            ti_item_t * item = ctx->data;
            ti_raw_t * raw;
            size_t dst_n;

            take_buffer(up->ctx.pk.data, (char **) &raw, &dst_n);
            ti_raw_init(raw, TI_VAL_MPDATA, dst_n);

            item->val = (ti_val_t *) raw;
            ctx->data = up->item;
            free(up);
            return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
        }
        return ok;
    }

    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_start_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not an array");
    case MF__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not an array");
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not an array");
    case MF__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not an array");
    case MF__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not an array");
    case MF__DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults` "
                "in "TI_MANIFEST", not an array");
    case MF__DEFAULTS_DEEP:
        return manifest__set_err(
                ctx,
                "expecting a `deep` value between 0 and %d, not an array",
                TI_MAX_DEEP_HINT);
    case MF__DEFAULTS_LOAD:
        return manifest__set_err(
                ctx,
                "expecting `load` to be true or false, not an array");
    case MF__DEFAULTS_ITEM:
        return manifest__set_err(
                ctx,
                "TODO: build array value"); /* TODO: mpdata */
    case MF__DEFAULTS_PACK:
        return reformat_start_array(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:
        return manifest__set_mode(ctx, MF__INCLUDES_ARR);
    case MF__INCLUDES_ARR:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `include` item, not an array");
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as `platform/architecture`, not an array");
    case MF__EXPOSES:
        return manifest__set_err(
                ctx,
                "expecting a map with functions to expose "
                "in "TI_MANIFEST", not an array");
    case MF__X:
        return manifest__set_err(
                ctx,
                "expecting a map as `expose` value, not an array");
    case MF__X_DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc`, not an array");
    case MF__X_DEFAULTS:
        return manifest__set_err(
                ctx,
                "expecting a map with `defaults`, not an array");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_end_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__DEFAULTS_PACK:
        return reformat_end_array(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES_ARR:
        return manifest__set_mode(ctx, MF__ROOT_MAP);

    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static yajl_callbacks manifest__callbacks = {
    manifest__json_null,
    manifest__json_boolean,
    manifest__json_integer,
    manifest__json_double,
    NULL,
    manifest__json_string,
    manifest__json_start_map,
    manifest__json_map_key,
    manifest__json_end_map,
    manifest__json_start_array,
    manifest__json_end_array
};

static int manifest__check_main(manifest__ctx_t * ctx)
{
    const char * pt = ctx->manifest->main;
    const char * file = pt;

    if (!pt)
        (void) manifest__set_err(ctx,
                "missing required `main` in "TI_MANIFEST);

    if (*pt == '\0')
        return manifest__set_err(ctx,
                "`main` in "TI_MANIFEST" must not be an empty string");

    if (!strx_is_printable(pt))
        return manifest__set_err(ctx,
                "`main` in "TI_MANIFEST" contains illegal characters");

    if (*pt == '/')
        return manifest__set_err(ctx,
                "`main` in "TI_MANIFEST" must not start with a `/`");

    for (++file; *file; ++file, ++pt)
        if (*pt == '.' && (*file == '.' || *file == '/'))
            return manifest__set_err(ctx,
                    "`main` in "TI_MANIFEST" must not contain `..` or `./` "
                    "to specify a relative path");

    return 1;  /* valid, success */
}

static int manifest__check_includes(manifest__ctx_t * ctx)
{
    vec_t * vec = ctx->manifest->includes;
    LOGC("SIZE: %d", vec->n);
    if (vec) for (vec_each(vec, const char, str))
    {
        const char * file = str;

        LOGC("include file: %s", str);

        if (*str == '\0')
            return manifest__set_err(ctx,
                    "include file must not be an empty string");

        if (!strx_is_printable(str))
            return manifest__set_err(ctx,
                    "include file contains illegal characters");

        if (*str == '/')
            return manifest__set_err(ctx,
                    "include file must not start with a `/`");

        for (++file; *file; ++file, ++str)
            if (*str == '.' && (*file == '.' || *file == '/'))
                return manifest__set_err(ctx,
                        "include file must not contain `..` or `./` "
                        "to specify a relative path");
    }

    return 1;  /* valid, success */
}

static int manifest__check_version(manifest__ctx_t * ctx)
{
    const char * pt = ctx->manifest->version;
    const size_t maxdigits = 5;
    size_t n, count = 3;

    if (!pt)
        (void) manifest__set_err(ctx,
                "missing required `version` in "TI_MANIFEST);

    if (*pt == '\0')
        return manifest__set_err(ctx,
                "`version` in "TI_MANIFEST" must not be an empty string");

    do
    {
        for (n=0; isdigit(*pt); ++pt)
            ++n;

        if (n == 0 || n > maxdigits)
            goto fail;

        if (*pt == '.')
            ++pt;
    }
    while (--count);

    return *pt == '\0';

fail:
    return manifest__set_err(ctx, "invalid `version` format in "TI_MANIFEST);
}

int ti_mod_manifest_read(
        ti_mod_manifest_t * manifest,
        char * source_err,
        const void * data,
        size_t n)
{
    yajl_handle hand;
    yajl_status stat = yajl_status_error;
    manifest__ctx_t ctx = {
            .manifest = manifest,
            .mode = MF__ROOT,
            .source_err = source_err,
            .data = NULL,
    };

    hand = yajl_alloc(&manifest__callbacks, NULL, &ctx);
    if (!hand)
        return stat;

    stat = yajl_parse(hand, data, n);

    if (stat == yajl_status_ok)
    {
        if (!manifest__check_main(&ctx) ||
            !manifest__check_version(&ctx) ||
            !manifest__check_includes(&ctx))
        {
            ti_mod_manifest_clear(manifest);
            stat = yajl_status_client_canceled;
        }
    }
    else
    {
        /* cleanup context on error */
        switch (ctx.mode)
        {
        case MF__DEFAULTS_PACK:
            /* In this context mode the unpack context and buffer needs to
             * be cleared
             */
            msgpack_sbuffer_destroy(&((manifest__up_t *) ctx.data)->buffer);
            free(ctx.data);
            break;
        default:
            break;
        }
    }

    yajl_free(hand);
    return stat;
}

int ti_mod_manifest_local(ti_mod_manifest_t * manifest, ti_module_t * module)
{
    ssize_t n;
    unsigned char * data = NULL;
    char tmp_err[TI_MODULE_MAX_ERR];
    int rc = -1;

    char * tmp_fn = fx_path_join(module->path, TI_MANIFEST);
    if (!tmp_fn || !fx_file_exist(tmp_fn))
        goto fail;

    data = fx_read(tmp_fn, &n);
    if (!data || ti_mod_manifest_read(manifest, tmp_err, data, (size_t) n))
    {
        log_warning("failed to read local "TI_MANIFEST" (%s)", tmp_err);
        goto fail;
    }

    rc = 0;
fail:
    LOGC("FAILED");
    free(data);
    free(tmp_fn);
    return rc;
}

/*
 * On errors, this will return `true` which in turn will trigger a new install.
 */
_Bool ti_mod_manifest_skip_install(
        ti_mod_manifest_t * manifest,
        ti_module_t * module)
{
    assert (manifest->main);
    assert (manifest->version);

    if (!module->manifest.version)
    {
        ti_mod_manifest_t tmpm = {0};
        _Bool skip_install;

        if (ti_mod_manifest_local(&tmpm, module))
            return false;

        skip_install = ti_version_cmp(tmpm.version, manifest->version) == 0;
        ti_mod_manifest_clear(&tmpm);
        return skip_install;
    }

    return ti_version_cmp(module->manifest.version, manifest->version);
}

int ti_mod_manifest_store(const char * module_path, void * data, size_t n)
{
    int rc;
    char * tmp_fn = fx_path_join(module_path, TI_MANIFEST);
    if (!tmp_fn)
        return -1;
    rc = fx_write(tmp_fn, data, n);
    free(tmp_fn);
    return rc;
}

static void maifest__item_destroy(ti_item_t * item)
{
    ti_val_drop((ti_val_t *) item->key);
    ti_val_drop(item->val);
    free(item);
}

void ti_mod_manifest_clear(ti_mod_manifest_t * manifest)
{
    free(manifest->main);
    free(manifest->version);
    free(manifest->doc);
    free(manifest->load);
    free(manifest->deep);
    vec_destroy(manifest->defaults, (vec_destroy_cb) maifest__item_destroy);
    vec_destroy(manifest->includes, (vec_destroy_cb) free);
    smap_destroy(manifest->exposes, (smap_destroy_cb) ti_mod_expose_destroy);
    memset(manifest, 0, sizeof(ti_mod_manifest_t));
}
