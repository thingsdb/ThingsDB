/*
 * ti/mod/manifest.c
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <ti/mod/expose.h>
#include <ti/mod/expose.t.h>
#include <ti/mod/manifest.h>
#include <ti/mod/manifest.t.h>
#include <ti/module.t.h>
#include <ti/version.h>
#include <tiinc.h>
#include <util/logger.h>
#include <util/osarch.h>
#include <util/strx.h>
#include <yajl/yajl_parse.h>

typedef enum
{
    MANIFEST__ROOT,
    MANIFEST__ROOT_MAP,
    MANIFEST__MAIN,
    MANIFEST__MAIN_MAP,          /* main may be a architecture map */
    MANIFEST__MAIN_OSARCH,
    MANIFEST__MAIN_NO_OSARCH,
    MANIFEST__VERSION,
    MANIFEST__DOC,
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
    return an == bn && memcmp(a, b, an) == 0;
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not null");
    case MANIFEST__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not null");
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a boolean");
    case MANIFEST__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not a boolean");
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a boolean");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_integer(void * data, long long i)
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    case MANIFEST__VERSION:
        if (!ctx->manifest->version)
            (void) asprintf(&ctx->manifest->version, "%lld", i);
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
                "in "TI_MANIFEST", not a number");
    default:
        return manifest__set_err(ctx, "unexpected error in "TI_MANIFEST);
    }
}

static int manifest__json_double(void * data, double d)
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a number");
    case MANIFEST__VERSION:
        if (!ctx->manifest->version)
            (void) asprintf(&ctx->manifest->version, "%g", d);
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
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
    case MANIFEST__VERSION:
        if (!ctx->manifest->version)
            ctx->manifest->version = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
    case MANIFEST__DOC:
        if (!ctx->manifest->doc)
            ctx->manifest->doc = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MANIFEST__ROOT_MAP);
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not a map");
    case MANIFEST__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not a map");
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
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
        if (manifest__key_equals(s, n, "version", 7))
            return manifest__set_mode(ctx, MANIFEST__VERSION);
        if (manifest__key_equals(s, n, "doc", 3))
            return manifest__set_mode(ctx, MANIFEST__DOC);
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
                "expecting a string as `platform/architecture` "
                "in "TI_MANIFEST", not an array");
    case MANIFEST__VERSION:
        return manifest__set_err(
                ctx,
                "expecting a string or number as `version` "
                "in "TI_MANIFEST", not an array");
    case MANIFEST__DOC:
        return manifest__set_err(
                ctx,
                "expecting a string as `doc` "
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
            .mode = MANIFEST__ROOT,
            .source_err = source_err,
    };

    hand = yajl_alloc(&manifest__callbacks, NULL, &ctx);
    if (!hand)
        return stat;

    stat = yajl_parse(hand, data, n);

    if (stat == yajl_status_ok)
    {
        if (!manifest__check_main(&ctx) ||
            !manifest__check_version(&ctx))
        {
            ti_mod_manifest_clear(manifest);
            stat = yajl_status_client_canceled;
        }
    }

    yajl_free(hand);
    return stat;
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
        ssize_t n;
        unsigned char * data = NULL;
        char tmp_err[TI_MODULE_MAX_ERR];
        _Bool skip_install = false;

        char * tmp_fn = fx_path_join(module->path, TI_MANIFEST);
        if (!tmp_fn || !fx_file_exist(tmp_fn))
            goto fail;

        data = fx_read(tmp_fn, &n);
        if (!data || ti_mod_manifest_read(&tmpm, tmp_err, data, (size_t) n))
            goto fail;

        skip_install = ti_version_cmp(tmpm.version, manifest->version) == 0;
        ti_mod_manifest_clear(&tmpm);
    fail:
        free(data);
        free(tmp_fn);
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

void ti_mod_manifest_clear(ti_mod_manifest_t * manifest)
{
    free(manifest->main);
    free(manifest->version);
    free(manifest->doc);
    free(manifest->load);
    free(manifest->deep);
    smap_destroy(manifest->exposes, (smap_destroy_cb) ti_mod_expose_destroy);
    memset(manifest, 0, sizeof(ti_mod_manifest_t));
}
