
/*
 * ti/modu/manifest.c
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <ti/item.h>
#include <ti/item.t.h>
#include <ti/modu/expose.h>
#include <ti/modu/expose.t.h>
#include <ti/modu/manifest.h>
#include <ti/modu/manifest.t.h>
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
#include <util/logger.h>
#include <util/mpjson.h>
#include <util/osarch.h>
#include <util/strx.h>
#include <yajl/yajl_parse.h>

#define manifest__err_alloc "allocation error while parsing "TI_MANIFEST

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
    MF__X_ARGMAP,
    MF__X_ARGMAP_ARR,
    MF__REQUIREMENTS,
    MF__REQUIREMENTS_ARR,
} manifest__ctx_mode_t;

typedef struct
{
    manifest__ctx_mode_t mode;
    ti_modu_manifest_t * manifest;
    char * source_err;          /* pointer to module->source_err which is
                                   limited by (TI_MODULE_MAX_ERR */
    void * data;                /* pointer to anything */
} manifest__ctx_t;

typedef struct
{
    mpjson_convert_t ctx;       /* pointer to unpack context */
    msgpack_sbuffer buffer;
    void * data;
} manifest__up_t;

static int manifest__set_err(
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

static int manifest__check_reqm(const char * str, size_t n)
{
    for (; n; --n, ++str)
        if (!isalnum(*str) &&
                *str != ' ' &&
                *str != '_' &&
                *str != '-' &&
                *str != ',' &&
                *str != '!' &&
                *str != '.' &&
                *str != '<' &&
                *str != '=' &&
                *str != '>' &&
                *str != '~')
            return 0;
    return 1;
}

static int manifest__default_item(manifest__ctx_t * ctx, void * val)
{
    ti_item_t * item = ctx->data;
    item->val = val;
    return val
            ? manifest__set_mode(ctx, MF__DEFAULTS_MAP)
            : manifest__set_err(ctx, manifest__err_alloc);
}

static int manifest__x_default_item(manifest__ctx_t * ctx, void * val)
{
    ti_modu_expose_t * expose = ctx->data;
    ti_item_t * item = VEC_last(expose->defaults);
    item->val = val;
    return val
            ? manifest__set_mode(ctx, MF__X_DEFAULTS_MAP)
            : manifest__set_err(ctx, manifest__err_alloc);
}

static int manifest__set_main(
        manifest__ctx_t * ctx,
        manifest__ctx_mode_t mode,
        const unsigned char * s,
        size_t n)
{
    if (!ctx->manifest->main)
    {
        ctx->manifest->main = strndup((const char *) s, n);
        ctx->manifest->is_py = ti_module_file_is_py((const char *) s, n);
    }
    return manifest__set_mode(ctx, mode);
}

static int manifest__start_pack(manifest__ctx_t * ctx, manifest__ctx_mode_t mode)
{
    manifest__up_t * up = calloc(1, sizeof(manifest__up_t));
    if (!up || mp_sbuffer_alloc_init(&up->buffer, 8192, sizeof(ti_raw_t)))
    {
        free(up);
        return -1;
    }

    msgpack_packer_init(&up->ctx.pk, &up->buffer, msgpack_sbuffer_write);

    up->data = ctx->data;  /* item or expose */
    ctx->data = up;

    (void) manifest__set_mode(ctx, mode);
    return 0;
}

typedef int (*manifest__pcb) (void *);

static int manifest__end_pack(manifest__ctx_t * ctx, manifest__pcb cb)
{
    manifest__up_t * up = ctx->data;
    int ok = cb(&up->ctx);
    if (ok && up->ctx.deep == 0)
    {
        ti_item_t * item = up->data;
        ti_raw_t * raw;
        size_t dst_n;

        take_buffer(up->ctx.pk.data, (char **) &raw, &dst_n);
        ti_raw_init(raw, TI_VAL_MPDATA, dst_n);

        item->val = (ti_val_t *) raw;
        ctx->data = item;
        free(up);
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    }
    return ok;
}

static int manifest__x_end_pack(manifest__ctx_t * ctx, manifest__pcb cb)
{
    manifest__up_t * up = ctx->data;
    int ok = cb(&up->ctx);
    if (ok && up->ctx.deep == 0)
    {
        ti_modu_expose_t * expose = up->data;
        ti_item_t * item = VEC_last(expose->defaults);
        ti_raw_t * raw;
        size_t dst_n;

        take_buffer(up->ctx.pk.data, (char **) &raw, &dst_n);
        ti_raw_init(raw, TI_VAL_MPDATA, dst_n);

        item->val = (ti_val_t *) raw;
        ctx->data = expose;
        free(up);
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    return ok;
}

static ti_item_t * manifest__make_item(
        manifest__ctx_t * ctx,
        const char * str,
        size_t n,
        vec_t ** defaults)
{
    ti_item_t * item;

    if (!strx_is_utf8n(str, n))
        return manifest__set_err(ctx,
                "default keys value must have valid UTF-8 encoding"), NULL;

    if (ti_is_reserved_key_strn(str, n))
        return manifest__set_err(ctx,
                "default keys `%c` is reserved"DOC_PROPERTIES,
                *str), NULL;

    if (*defaults) for (vec_each(*defaults, ti_item_t, item))
        if (ti_raw_eq_strn(item->key, str, n))
            return manifest__set_err(ctx,
                    "key `%.*s` exists more than once in `defaults`",
                    (int) n, str), NULL;

    item = malloc(sizeof(ti_item_t));
    if (!item)
        return manifest__set_err(ctx, manifest__err_alloc), NULL;

    item->val = NULL;
    item->key = ti_name_is_valid_strn(str, n)
           ? (ti_raw_t *) ti_names_get(str, n)
           : ti_str_create(str, n);

    if (!item->key || vec_push_create(defaults, item))
    {
        ti_val_drop((ti_val_t *) item->key);
        free(item);
        return manifest__set_err(ctx, manifest__err_alloc), NULL;
    }
    return item;
}


#define manifest__err_root(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a map in "TI_MANIFEST", "__notv);


#define manifest__err_main(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string or map as `main` in "TI_MANIFEST", "__notv);


#define manifest__err_osarch(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string as `platform/architecture` in "TI_MANIFEST", "__notv);


#define manifest__err_version(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string or number as `version` in "TI_MANIFEST", "__notv);


#define manifest__err_doc(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string as `doc` in "TI_MANIFEST", "__notv);


#define manifest__err_defaults(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a map with `defaults` in "TI_MANIFEST", "__notv);


#define manifest__err_deep(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a `deep` value between 0 and %d in "TI_MANIFEST", "__notv, TI_MAX_DEEP_HINT);


#define manifest__err_load(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting `load` to be true or false in "TI_MANIFEST", "__notv);


#define manifest__err_includes(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting an array with `includes` in "TI_MANIFEST", "__notv);


#define manifest__err_incl_arr(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string or map as `include` item in "TI_MANIFEST", "__notv);


#define manifest__err_exposes(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a map with functions to expose in "TI_MANIFEST", "__notv);


#define manifest__err_x(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a map as `expose` value in "TI_MANIFEST", "__notv);


#define manifest__err_argmap(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting an array or null as `argmap` value in "TI_MANIFEST", "__notv);


#define manifest__err_am_arr(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string as `argmap` item in "TI_MANIFEST", "__notv);


#define manifest__err_reqm(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting an array as `requirements` value in "TI_MANIFEST", "__notv);


#define manifest__err_reqm_arr(__ctx, __notv) \
    manifest__set_err(__ctx, "expecting a string as `requirements` item in "TI_MANIFEST", "__notv);


#define manifest__err_unexpected(__ctx) \
    manifest__set_err(__ctx, "unexpected error in "TI_MANIFEST);


#define TI_NNULL "not null"

static int manifest__json_null(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, TI_NNULL);
    case MF__MAIN:              return manifest__err_main(ctx, TI_NNULL);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, TI_NNULL);
    case MF__VERSION:           return manifest__err_version(ctx, TI_NNULL);
    case MF__DOC:               return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, TI_NNULL);
    case MF__DEFAULTS_DEEP:     return manifest__err_deep(ctx, TI_NNULL);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, TI_NNULL);
    case MF__DEFAULTS_ITEM:
        return manifest__default_item(ctx, ti_nil_get());
    case MF__DEFAULTS_PACK:
        return reformat_null(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NNULL);
    case MF__INCLUDES_ARR:      return manifest__err_incl_arr(ctx, TI_NNULL);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, TI_NNULL);
    case MF__EXPOSES:           return manifest__err_exposes(ctx, TI_NNULL);
    case MF__X:                 return manifest__err_x(ctx, TI_NNULL);
    case MF__X_DOC:             return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, TI_NNULL);
    case MF__X_DEFAULTS_DEEP:   return manifest__err_deep(ctx, TI_NNULL);
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, TI_NNULL);
    case MF__X_DEFAULTS_ITEM:
        return manifest__x_default_item(ctx, ti_nil_get());
    case MF__X_DEFAULTS_PACK:
        return reformat_null(&((manifest__up_t *) ctx->data)->ctx);
    case MF__X_ARGMAP:          return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, TI_NNULL);
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NNULL);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, TI_NNULL);
    default:                    return manifest__err_unexpected(ctx);
    }
}

#define TI_NBOOL "not a boolean"

static int manifest__json_boolean(void * data, int boolean)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, TI_NBOOL);
    case MF__MAIN:              return manifest__err_main(ctx, TI_NBOOL);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, TI_NBOOL);
    case MF__VERSION:           return manifest__err_version(ctx, TI_NBOOL);
    case MF__DOC:               return manifest__err_doc(ctx, TI_NBOOL);
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, TI_NBOOL);
    case MF__DEFAULTS_DEEP:     return manifest__err_deep(ctx, TI_NBOOL);
    case MF__DEFAULTS_LOAD:
        if (!ctx->manifest->load)
        {
            ctx->manifest->load = malloc(sizeof(_Bool));
            if (ctx->manifest->load)
                *ctx->manifest->load = boolean;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_ITEM:
        return manifest__default_item(ctx, ti_vbool_get(boolean));
    case MF__DEFAULTS_PACK:
        return reformat_boolean(&((manifest__up_t *) ctx->data)->ctx, boolean);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NBOOL);
    case MF__INCLUDES_ARR:      return manifest__err_incl_arr(ctx, TI_NBOOL);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, TI_NBOOL);
    case MF__EXPOSES:           return manifest__err_exposes(ctx, TI_NBOOL);
    case MF__X:                 return manifest__err_x(ctx, TI_NBOOL);
    case MF__X_DOC:             return manifest__err_doc(ctx, TI_NBOOL);
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, TI_NBOOL);
    case MF__X_DEFAULTS_DEEP:   return manifest__err_deep(ctx, TI_NBOOL);
    case MF__X_DEFAULTS_LOAD:
    {
        ti_modu_expose_t * expose = ctx->data;
        if (!expose->load)
        {
            expose->load = malloc(sizeof(_Bool));
            if (expose->load)
                *expose->load = boolean;
        }
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_ITEM:
        return manifest__x_default_item(ctx, ti_vbool_get(boolean));
    case MF__X_DEFAULTS_PACK:
        return reformat_boolean(&((manifest__up_t *) ctx->data)->ctx, boolean);
    case MF__X_ARGMAP:          return manifest__err_argmap(ctx, TI_NBOOL);
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, TI_NBOOL);
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NBOOL);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, TI_NBOOL);
    default:                    return manifest__err_unexpected(ctx);
    }
}

#define TI_NNUM "not a number"

static int manifest__json_integer(void * data, long long i)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, TI_NNUM);
    case MF__MAIN:              return manifest__err_main(ctx, TI_NNUM);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, TI_NNUM);
    case MF__VERSION:
        if (!ctx->manifest->version)
            if (asprintf(&ctx->manifest->version, "%lld", i) < 0)
                return manifest__set_err(ctx, manifest__err_alloc);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DOC:               return manifest__err_doc(ctx, TI_NNUM);
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, TI_NNUM);
    case MF__DEFAULTS_DEEP:
        if (i < 0 || i > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d "
                    "in "TI_MANIFEST", not %lld",
                    TI_MAX_DEEP_HINT, i);
        if (!ctx->manifest->deep)
        {
            ctx->manifest->deep = malloc(sizeof(uint8_t));
            if (ctx->manifest->deep)
                *ctx->manifest->deep = (uint8_t) i;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, TI_NNUM);
    case MF__DEFAULTS_ITEM:
        return manifest__default_item(ctx, ti_vint_create(i));
    case MF__DEFAULTS_PACK:
        return reformat_integer(&((manifest__up_t *) ctx->data)->ctx, i);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NNUM);
    case MF__INCLUDES_ARR:      return manifest__err_incl_arr(ctx, TI_NNUM);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, TI_NNUM);
    case MF__EXPOSES:           return manifest__err_exposes(ctx, TI_NNUM);
    case MF__X:                 return manifest__err_x(ctx, TI_NNUM);
    case MF__X_DOC:             return manifest__err_doc(ctx, TI_NNUM);
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, TI_NNUM);
    case MF__X_DEFAULTS_DEEP:
    {
        ti_modu_expose_t * expose = ctx->data;
        if (i < 0 || i > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d "
                    "in "TI_MANIFEST", not %lld",
                    TI_MAX_DEEP_HINT, i);
        if (!expose->deep)
        {
            expose->deep = malloc(sizeof(uint8_t));
            if (expose->deep)
                *expose->deep = (uint8_t) i;
        }
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, TI_NNUM);
    case MF__X_DEFAULTS_ITEM:
        return manifest__x_default_item(ctx, ti_vint_create(i));
    case MF__X_DEFAULTS_PACK:
        return reformat_integer(&((manifest__up_t *) ctx->data)->ctx, i);
    case MF__X_ARGMAP:          return manifest__err_argmap(ctx, TI_NNUM);
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, TI_NNUM);
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NNUM);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, TI_NNUM);
    default:                    return manifest__err_unexpected(ctx);
    }
}

static int manifest__json_double(void * data, double d)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, TI_NNUM);
    case MF__MAIN:              return manifest__err_main(ctx, TI_NNUM);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, TI_NNUM);
    case MF__VERSION:
        if (!ctx->manifest->version)
            if (asprintf(&ctx->manifest->version, "%g", d) < 0)
                return manifest__set_err(ctx, manifest__err_alloc);
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DOC:               return manifest__err_doc(ctx, TI_NNUM);
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, TI_NNUM);
    case MF__DEFAULTS_DEEP:
        if (d < 0 || d > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d "
                    "in "TI_MANIFEST", not %f",
                    TI_MAX_DEEP_HINT, d);
        if (!ctx->manifest->deep)
        {
            ctx->manifest->deep = malloc(sizeof(uint8_t));
            if (ctx->manifest->deep)
                *ctx->manifest->deep = (uint8_t) d;
        }
        return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, TI_NNUM);
    case MF__DEFAULTS_ITEM:
        return manifest__default_item(ctx, ti_vfloat_create(d));
    case MF__DEFAULTS_PACK:
        return reformat_double(&((manifest__up_t *) ctx->data)->ctx, d);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NNUM);
    case MF__INCLUDES_ARR:      return manifest__err_incl_arr(ctx, TI_NNUM);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, TI_NNUM);
    case MF__EXPOSES:           return manifest__err_exposes(ctx, TI_NNUM);
    case MF__X:                 return manifest__err_x(ctx, TI_NNUM);
    case MF__X_DOC:             return manifest__err_doc(ctx, TI_NNUM);
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, TI_NNUM);
    case MF__X_DEFAULTS_DEEP:
    {
        ti_modu_expose_t * expose = ctx->data;
        if (d < 0 || d > TI_MAX_DEEP_HINT)
            return manifest__set_err(
                    ctx,
                    "expecting a `deep` value between 0 and %d "
                    "in "TI_MANIFEST", not %f",
                    TI_MAX_DEEP_HINT, d);
        if (!expose->deep)
        {
            expose->deep = malloc(sizeof(uint8_t));
            if (expose->deep)
                *expose->deep = (uint8_t) d;
        }
        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    }
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, TI_NNUM);
    case MF__X_DEFAULTS_ITEM:
        return manifest__x_default_item(ctx, ti_vfloat_create(d));
    case MF__X_DEFAULTS_PACK:
        return reformat_double(&((manifest__up_t *) ctx->data)->ctx, d);
    case MF__X_ARGMAP:          return manifest__err_argmap(ctx, TI_NNUM);
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, TI_NNUM);
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NNUM);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, TI_NNUM);
    default:                    return manifest__err_unexpected(ctx);
    }
}

#define TI_NSTR "not a string"

static int manifest__json_string(
        void * data,
        const unsigned char * s,
        size_t n)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, TI_NSTR);
    case MF__MAIN:
        return manifest__set_main(ctx, MF__ROOT_MAP, s, n);
    case MF__MAIN_OSARCH:
        return manifest__set_main(ctx, MF__MAIN_MAP, s, n);
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
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, TI_NSTR);
    case MF__DEFAULTS_DEEP:     return manifest__err_deep(ctx, TI_NSTR);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, TI_NSTR);
    case MF__DEFAULTS_ITEM:
        return manifest__default_item(ctx, ti_str_create((const char *) s, n));
    case MF__DEFAULTS_PACK:
        return reformat_string(&((manifest__up_t *) ctx->data)->ctx, s, n);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NSTR);
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
    case MF__EXPOSES:           return manifest__err_exposes(ctx, TI_NSTR);
    case MF__X:                 return manifest__err_x(ctx, TI_NSTR);
    case MF__X_DOC:
    {
        ti_modu_expose_t * expose = ctx->data;
        if (!expose->doc)
            expose->doc = strndup((const char *) s, n);
        return manifest__set_mode(ctx, MF__X_MAP);
    }
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, TI_NSTR);
    case MF__X_DEFAULTS_DEEP:   return manifest__err_deep(ctx, TI_NSTR);
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, TI_NSTR);
    case MF__X_DEFAULTS_ITEM:
        return manifest__x_default_item(
                ctx,
                ti_str_create((const char *) s, n));
    case MF__X_DEFAULTS_PACK:
        return reformat_string(&((manifest__up_t *) ctx->data)->ctx, s, n);
    case MF__X_ARGMAP:          return manifest__err_argmap(ctx, TI_NSTR);
    case MF__X_ARGMAP_ARR:
    {
        const char * str = (const char *) s;
        ti_modu_expose_t * expose = ctx->data;
        ti_item_t * item;

        if (!strx_is_utf8n(str, n))
            return manifest__set_err(ctx,
                    "argmap value in "TI_MANIFEST" must have "
                    "valid UTF-8 encoding");

        if (ti_is_reserved_key_strn(str, n))
            return manifest__set_err(ctx,
                    "argmap value `%c` in "TI_MANIFEST" is reserved"
                    DOC_PROPERTIES,
                    *str);

        if (expose->argmap) for (vec_each(expose->argmap, ti_item_t, item))
            if (ti_raw_eq_strn(item->key, str, n))
                return manifest__set_err(ctx,
                        "value `%.*s` exists more than once as `argmap` "
                        "item in "TI_MANIFEST,
                        (int) n, str);

        item = malloc(sizeof(ti_item_t));
        if (!item)
            return manifest__set_err(ctx, manifest__err_alloc);

        item->val = NULL;
        item->key = ti_name_is_valid_strn(str, n)
                ? (ti_raw_t *) ti_names_get(str, n)
                : ti_str_create(str, n);

        return item->key && 0 == vec_push(&expose->argmap, item);
    }
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NNUM);
    case MF__REQUIREMENTS_ARR:
    {
        const char * str = (const char *) s;
        char * reqm;

        if (!manifest__check_reqm(str, n))
            return manifest__set_err(ctx,
                    "invalid characters found in one of the "
                    "requirements in "TI_MANIFEST);

        reqm = strndup(str, n);
        return reqm && 0 == vec_push(&ctx->manifest->requirements, reqm);
    }
    default:                    return manifest__err_unexpected(ctx);
    }
}

#define TI_NOMAP "not a map"

static int manifest__json_start_map(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:
        return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__MAIN:
        return manifest__set_mode(ctx, MF__MAIN_MAP);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, TI_NOMAP);
    case MF__VERSION:           return manifest__err_version(ctx, TI_NOMAP);
    case MF__DOC:               return manifest__err_doc(ctx, TI_NOMAP);
    case MF__DEFAULTS:          return manifest__set_mode(ctx, MF__DEFAULTS_MAP);
    case MF__DEFAULTS_DEEP:     return manifest__err_deep(ctx, TI_NOMAP);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, TI_NOMAP);
    case MF__DEFAULTS_ITEM:
        if (manifest__start_pack(ctx, MF__DEFAULTS_PACK))
            return manifest__set_err(ctx, manifest__err_alloc);
        /*fall through */
    case MF__DEFAULTS_PACK:
        return reformat_start_map(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:          return manifest__err_includes(ctx, TI_NOMAP);
    case MF__INCLUDES_ARR:      return manifest__set_mode(ctx, MF__INCLUDES_MAP);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, TI_NOMAP);
    case MF__EXPOSES:
        return ctx->manifest->exposes || (ctx->manifest->exposes = smap_create())
            ? manifest__set_mode(ctx, MF__EXPOSES_MAP)
            : manifest__set_err(ctx, manifest__err_alloc);
    case MF__X:                 return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_DOC:             return manifest__err_doc(ctx, TI_NOMAP);
    case MF__X_DEFAULTS:        return manifest__set_mode(ctx, MF__X_DEFAULTS_MAP);
    case MF__X_DEFAULTS_DEEP:   return manifest__err_deep(ctx, TI_NOMAP);
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, TI_NOMAP);
    case MF__X_DEFAULTS_ITEM:
        if (manifest__start_pack(ctx, MF__X_DEFAULTS_PACK))
            return manifest__set_err(ctx, manifest__err_alloc);
        /*fall through */
    case MF__X_DEFAULTS_PACK:
        return reformat_start_map(&((manifest__up_t *) ctx->data)->ctx);
    case MF__X_ARGMAP:          return manifest__err_argmap(ctx, TI_NOMAP);
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, TI_NOMAP);
    case MF__REQUIREMENTS:      return manifest__err_reqm(ctx, TI_NOMAP);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, TI_NOMAP);
    default:                    return manifest__err_unexpected(ctx);
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
        if (manifest__key_equals(s, n, "requirements", 12))
            return manifest__set_mode(ctx, MF__REQUIREMENTS);

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
            ti_item_t * item = manifest__make_item(
                    ctx,
                    (const char *) s,
                    n,
                    &ctx->manifest->defaults);
            if (item)
            {
                ctx->data = item;
                return manifest__set_mode(ctx, MF__DEFAULTS_ITEM);
            }
            return 0;  /* failed, error has been set */
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
        ti_modu_expose_t * expose = ti_modu_expose_create();
        if (!expose ||
            smap_addn(ctx->manifest->exposes, (const char *) s, n, expose))
            return manifest__set_err(
                    ctx,
                    "double expose function in "TI_MANIFEST);
        ctx->data = expose;
        return manifest__set_mode(ctx, MF__X);
    }
    case MF__X_MAP:
        if (manifest__key_equals(s, n, "doc", 3))
            return manifest__set_mode(ctx, MF__X_DOC);
        if (manifest__key_equals(s, n, "defaults", 8))
            return manifest__set_mode(ctx, MF__X_DEFAULTS);
        if (manifest__key_equals(s, n, "argmap", 6))
            return manifest__set_mode(ctx, MF__X_ARGMAP);
        return manifest__set_err(
                ctx,
                "unsupported key `%.*s` used in exposed "
                "function in "TI_MANIFEST,
                n, s);
    case MF__X_DEFAULTS_MAP:
        if (manifest__key_equals(s, n, "deep", 4))
            return manifest__set_mode(ctx, MF__X_DEFAULTS_DEEP);
        if (manifest__key_equals(s, n, "load", 4))
            return manifest__set_mode(ctx, MF__X_DEFAULTS_LOAD);
        {
            ti_modu_expose_t * expose = ctx->data;
            ti_item_t * item = manifest__make_item(
                    ctx,
                    (const char *) s,
                    n,
                    &expose->defaults);
            return item ? manifest__set_mode(ctx, MF__X_DEFAULTS_ITEM) : 0;
        }
    case MF__X_DEFAULTS_PACK:
        return reformat_map_key(&((manifest__up_t *) ctx->data)->ctx, s, n);
    default:
        return manifest__err_unexpected(ctx);
    }
}

static int manifest__json_end_map(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT_MAP:      return manifest__set_mode(ctx, MF__ROOT);
    case MF__MAIN_MAP:      return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS_MAP:  return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__DEFAULTS_PACK: return manifest__end_pack(ctx, reformat_end_map);
    case MF__INCLUDES_MAP:  return manifest__set_mode(ctx, MF__INCLUDES_ARR);
    case MF__EXPOSES_MAP:   return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__X_MAP:         return manifest__set_mode(ctx, MF__EXPOSES_MAP);
    case MF__X_DEFAULTS_MAP:
                            return manifest__set_mode(ctx, MF__X_MAP);
    case MF__X_DEFAULTS_PACK:
                            return manifest__x_end_pack(ctx, reformat_end_map);
    default:                return manifest__err_unexpected(ctx);
    }
}

#define NOARRAY "not an array"

static int manifest__json_start_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__ROOT:              return manifest__err_root(ctx, NOARRAY);
    case MF__MAIN:              return manifest__err_main(ctx, NOARRAY);
    case MF__MAIN_OSARCH:
    case MF__MAIN_NO_OSARCH:    return manifest__err_osarch(ctx, NOARRAY);
    case MF__VERSION:           return manifest__err_version(ctx, NOARRAY);
    case MF__DOC:               return manifest__err_doc(ctx, NOARRAY);
    case MF__DEFAULTS:          return manifest__err_defaults(ctx, NOARRAY);
    case MF__DEFAULTS_DEEP:     return manifest__err_deep(ctx, NOARRAY);
    case MF__DEFAULTS_LOAD:     return manifest__err_load(ctx, NOARRAY);
    case MF__DEFAULTS_ITEM:
        if (manifest__start_pack(ctx, MF__DEFAULTS_PACK))
            return manifest__set_err(ctx, manifest__err_alloc);
        /*fall through */
    case MF__DEFAULTS_PACK:
        return reformat_start_array(&((manifest__up_t *) ctx->data)->ctx);
    case MF__INCLUDES:          return manifest__set_mode(ctx, MF__INCLUDES_ARR);
    case MF__INCLUDES_ARR:      return manifest__err_incl_arr(ctx, NOARRAY);
    case MF__INCLUDES_OSARCH:
    case MF__INCLUDES_NO_OSARCH:
                                return manifest__err_osarch(ctx, NOARRAY);
    case MF__EXPOSES:           return manifest__err_exposes(ctx, NOARRAY);
    case MF__X:                 return manifest__err_x(ctx, NOARRAY);
    case MF__X_DOC:             return manifest__err_doc(ctx, NOARRAY);
    case MF__X_DEFAULTS:        return manifest__err_defaults(ctx, NOARRAY);
    case MF__X_DEFAULTS_DEEP:   return manifest__err_deep(ctx, NOARRAY);
    case MF__X_DEFAULTS_LOAD:   return manifest__err_load(ctx, NOARRAY);
    case MF__X_DEFAULTS_ITEM:
        if (manifest__start_pack(ctx, MF__X_DEFAULTS_PACK))
            return manifest__set_err(ctx, manifest__err_alloc);
        /*fall through */
    case MF__X_DEFAULTS_PACK:
        return reformat_start_array(&((manifest__up_t *) ctx->data)->ctx);
    case MF__X_ARGMAP:
    {
        ti_modu_expose_t * expose = ctx->data;
        expose->argmap = vec_new(3);
        return expose->argmap
                ? manifest__set_mode(ctx, MF__X_ARGMAP_ARR)
                : manifest__set_err(ctx, manifest__err_alloc);
    }
    case MF__X_ARGMAP_ARR:      return manifest__err_am_arr(ctx, NOARRAY);
    case MF__REQUIREMENTS:
        if (!ctx->manifest->requirements)
        {
            ctx->manifest->requirements = vec_new(0);
            if (!ctx->manifest->requirements)
                return manifest__set_err(ctx, manifest__err_alloc);
        }
        return manifest__set_mode(ctx, MF__REQUIREMENTS_ARR);
    case MF__REQUIREMENTS_ARR:  return manifest__err_reqm_arr(ctx, NOARRAY);
    default:                    return manifest__err_unexpected(ctx);
    }
}

static int manifest__json_end_array(void * data)
{
    manifest__ctx_t * ctx = data;
    switch (ctx->mode)
    {
    case MF__DEFAULTS_PACK: return manifest__end_pack(ctx, reformat_end_array);
    case MF__INCLUDES_ARR:  return manifest__set_mode(ctx, MF__ROOT_MAP);
    case MF__X_DEFAULTS_PACK:
                            return manifest__x_end_pack(ctx, reformat_end_array);
    case MF__X_ARGMAP_ARR:  return manifest__set_mode(ctx, MF__X_MAP);
    case MF__REQUIREMENTS_ARR:
                            return manifest__set_mode(ctx, MF__ROOT_MAP);
    default:                return manifest__err_unexpected(ctx);
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
        return manifest__set_err(ctx,
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
    if (vec) for (vec_each(vec, const char, str))
    {
        const char * file = str;

        if (*str == '\0')
            return manifest__set_err(ctx,
                "include file in "TI_MANIFEST" must not be an empty string");

        if (!strx_is_printable(str))
            return manifest__set_err(ctx,
                "include file in "TI_MANIFEST" contains illegal characters");

        if (*str == '/')
            return manifest__set_err(ctx,
                "include file in "TI_MANIFEST" must not start with a `/`");

        for (++file; *file; ++file, ++str)
            if (*str == '.' && (*file == '.' || *file == '/'))
                return manifest__set_err(ctx,
                    "include file in "TI_MANIFEST" must not contain `..` "
                    "or `./` to specify a relative path");
    }

    return 1;  /* valid, success */
}

static inline int manifest__has_key(vec_t * defaults, ti_raw_t * key)
{
    for (vec_each(defaults, ti_item_t, item))
        if (ti_raw_eq(key, item->key))
            return 1;
    return 0;
}

/*
 * Return 0 on success, -1 allocation error
 */
static int manifest__exposes_cb(
        ti_modu_expose_t * expose,
        ti_modu_manifest_t * manifest)
{
    if (manifest->defaults)
    {
        if (!expose->defaults)
        {
            expose->defaults = vec_new(manifest->defaults->n);
            if (!expose->defaults)
                return -1;
        }

        for (vec_each(manifest->defaults, ti_item_t, item_def))
        {
            ti_item_t * item;
            if (manifest__has_key(expose->defaults, item_def->key))
                continue;

            item = ti_item_create(item_def->key, item_def->val);
            if (!item || vec_push(&expose->defaults, item))
            {
                free(item);
                return -1;
            }
            ti_incref(item->key);
            ti_incref(item->val);
        }
    }

    if (manifest->deep && !expose->deep)
    {
        expose->deep = malloc(sizeof(uint8_t));
        if (!expose->deep)
            return -1;
        *expose->deep = *manifest->deep;
    }

    if (manifest->load && !expose->load)
    {
        expose->load = malloc(sizeof(_Bool));
        if (!expose->load)
            return -1;
        *expose->load = *manifest->load;
    }

    if (expose->argmap) for (vec_each(expose->argmap, ti_item_t, item_map))
    {
        if (expose->defaults) for (vec_each(expose->defaults, ti_item_t, item))
        {
            if (ti_raw_eq(item_map->key, item->key))
            {
                item_map->val = item->val;
                ti_incref(item->val);
                break;
            }
        }

        if (!item_map->val)
            item_map->val = (ti_val_t *) ti_nil_get();
    }

    return 0;
}

static int manifest__check_exposes(manifest__ctx_t * ctx)
{
    return ctx->manifest->exposes && smap_values(
        ctx->manifest->exposes,
        (smap_val_cb) manifest__exposes_cb,
        ctx->manifest)
            ? manifest__set_err(ctx, manifest__err_alloc)
            : 1;  /* success */
}

static int manifest__check_py(manifest__ctx_t * ctx)
{
    return !ctx->manifest->is_py && ctx->manifest->requirements
            ? manifest__set_err(ctx,
                    "requirements in "TI_MANIFEST" are only allowed "
                    "with Python modules")
            : 1;  /* success */
}

static int manifest__check_version(manifest__ctx_t * ctx)
{
    const char * pt = ctx->manifest->version;
    const size_t maxdigits = 5;
    size_t n, count = 3;

    if (!pt)
        return manifest__set_err(ctx,
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

        if (*pt == '\0')
            return 1;  /* valid, success */

        if (*pt == '.')
            ++pt;
    }
    while (--count);

    if (*pt == '\0')
        return 1;  /* valid, success */

fail:
    return manifest__set_err(ctx, "invalid `version` format in "TI_MANIFEST);
}

int ti_modu_manifest_read(
        ti_modu_manifest_t * manifest,
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
            !manifest__check_includes(&ctx) ||
            !manifest__check_exposes(&ctx) ||
            !manifest__check_py(&ctx))
        {
            stat = yajl_status_error;

            /* clear manifest */
            ti_modu_manifest_clear(manifest);
        }
    }
    else
    {
        /* allocation error if no error is set */
        if (*source_err == '\0')
            (void) manifest__set_err(&ctx, manifest__err_alloc);

        /* cleanup context on error */
        if (ctx.mode == MF__DEFAULTS_PACK)
        {
            /*
             * In this context mode the unpack context and buffer needs to
             * be cleared
             */
            msgpack_sbuffer_destroy(&((manifest__up_t *) ctx.data)->buffer);
            free(ctx.data);
        }

        /* clear manifest */
        ti_modu_manifest_clear(manifest);
    }

    yajl_free(hand);
    return stat;
}

int ti_modu_manifest_local(ti_modu_manifest_t * manifest, ti_module_t * module)
{
    ssize_t n;
    unsigned char * data = NULL;
    char tmp_err[TI_MODULE_MAX_ERR];
    int rc = -1;

    char * tmp_fn = fx_path_join(module->path, TI_MANIFEST);
    if (!tmp_fn || !fx_file_exist(tmp_fn))
        goto fail;

    data = fx_read(tmp_fn, &n);
    if (!data || ti_modu_manifest_read(manifest, tmp_err, data, (size_t) n))
    {
        log_warning("failed to read local "TI_MANIFEST" (%s)", tmp_err);
        goto fail;
    }

    rc = 0;
fail:
    free(data);
    free(tmp_fn);
    return rc;
}

/*
 * On errors, this will return `true` which in turn will trigger a new install.
 */
_Bool ti_modu_manifest_skip_install(
        ti_modu_manifest_t * manifest,
        ti_module_t * module)
{
    assert (manifest->main);
    assert (manifest->version);

    if (!module->manifest.version)
    {
        ti_modu_manifest_t tmpm = {0};
        _Bool skip_install;

        if (ti_modu_manifest_local(&tmpm, module))
            return false;

        skip_install = ti_version_cmp(tmpm.version, manifest->version) == 0;
        ti_modu_manifest_clear(&tmpm);
        return skip_install;
    }

    return ti_version_cmp(module->manifest.version, manifest->version) == 0;
}

int ti_modu_manifest_store(const char * module_path, void * data, size_t n)
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

void ti_modu_manifest_clear(ti_modu_manifest_t * manifest)
{
    free(manifest->main);
    free(manifest->version);
    free(manifest->doc);
    free(manifest->load);
    free(manifest->deep);
    vec_destroy(manifest->defaults, (vec_destroy_cb) maifest__item_destroy);
    vec_destroy(manifest->includes, (vec_destroy_cb) free);
    vec_destroy(manifest->requirements, (vec_destroy_cb) free);
    smap_destroy(manifest->exposes, (smap_destroy_cb) ti_modu_expose_destroy);
    memset(manifest, 0, sizeof(ti_modu_manifest_t));
}

