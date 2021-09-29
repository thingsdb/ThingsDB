/*
 * ti/mod/manifest.c
 */
#include <stdarg.h>
#include <stdio.h>
#include <ti/mod/manifest.t.h>
#include <ti/mod/expose.t.h>
#include <ti/mod/expose.h>
#include <ti/module.t.h>
#include <tiinc.h>
#include <yajl/yajl_parse.h>
#include <util/osarch.h>

typedef enum
{
    MANIFEST__ROOT,
    MANIFEST__ROOT_MAP,
    MANIFEST__MAIN,
    MANIFEST__MAIN_MAP,          /* main may be a architecture map */
    MANIFEST__MAIN_OSARCH,
    MANIFEST__MAIN_NO_OSARCH,
    MANIFEST__DEFAULTS,
} manifest__ctx_mode_t;

typedef struct
{
    manifest__ctx_mode_t mode;
    ti_mod_manifest_t * manifest;
    char * source_err;         /* pointer to module->source_err which is
                                  limited by (TI_MODULE_MAX_ERR */
} manifest__ctx_t;

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
    return an == bn && memcmp(a, b, 4) == 0;
}

static int manifest__json_null(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not null");
    case MANIFEST__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not null");
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not null");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_boolean(void * data, int UNUSED(boolean))
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a boolean");
    case MANIFEST__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a boolean");
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not a boolean");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_integer(void * data, long long UNUSED(i))
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a number");
    case MANIFEST__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a number");
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_double(void * data, double UNUSED(d))
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a number");
    case MANIFEST__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not a number");
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_string(
        void * data,
        const unsigned char * s,
        size_t n)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not a string");
    case MANIFEST__MAIN:
        if (!ctx->manifest->main)
            ctx->manifest->main = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    case MANIFEST__MAIN_OSARCH:
        if (!ctx->manifest->main)
            ctx->manifest->main = strndup((const char *) s, n);
        /* fall through */
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_mode(ctx, MANIFEST__MAIN_MAP);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_start_map(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    case MANIFEST__MAIN:
        return manifest__set_mode(ctx, MANIFEST__MAIN_MAP);
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not a map");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_map_key(
        void * data,
        const unsigned char * s,
        size_t n)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT_MAP:
        if (manifest__key_equals(s, n, "main", 4))
            return manifest__set_mode(ctx, MANIFEST__MAIN);
        if (manifest__key_equals(s, n, "defaults", 4))
            return manifest__set_mode(ctx, MANIFEST__DEFAULTS);
        return manifest__set_err(
                ctx, "unsupported key `%.*s` in "TI_MANIFEST,
                n, s);
    case MANIFEST__MAIN_MAP:
    {
        const char * osarch = osarch_get();
        size_t sz = strlen(osarch);
        if (manifest__key_equals(s, n, osarch, sz))
            return manifest__set_mode(ctx, MANIFEST__MAIN_OSARCH);
        return manifest__set_mode(ctx, MANIFEST__MAIN_NO_OSARCH);
    }
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);

    }
}

static int manifest__json_end_map(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT_MAP:
        return manifest__set_mode(ctx, MANIFEST__ROOT);
    case MANIFEST__MAIN_MAP:
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_start_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MANIFEST__ROOT:
        return manifest__set_err(
                ctx, "expecting a map in "TI_MANIFEST", not an array");
    case MANIFEST__MAIN:
        return manifest__set_err(
                ctx,
                "expecting a string or map as `main` "
                "in "TI_MANIFEST", not an array");
    case MANIFEST__MAIN_OSARCH:
    case MANIFEST__MAIN_NO_OSARCH:
        return manifest__set_err(
                ctx,
                "expecting a string as as `platform/architecture` "
                "in "TI_MANIFEST", not an array");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_end_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
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

int ti_mod_manifest_read(
        ti_mod_manifest_t * manifest,
        const void * data,
        size_t n)
{
    yajl_handle hand;
    yajl_status stat = yajl_status_error;
    manifest__ctx_t ctx = {
            .manifest = manifest,
            .mode = MANIFEST__ROOT,
    };

    hand = yajl_alloc(&manifest__callbacks, NULL, &ctx);
    if (!hand)
        return stat;

    stat = yajl_parse(hand, data, n);

    yajl_free(hand);
    return stat;
}

void ti_mod_manifest_clear(ti_mod_manifest_t * manifest)
{
    free(manifest->main);
    free(manifest->version);
    free(manifest->doc);
    free(manifest->load);
    free(manifest->deep);

    smap_destroy(manifest->exposes, (smap_destroy_cb) ti_mod_expose_destroy);
}
