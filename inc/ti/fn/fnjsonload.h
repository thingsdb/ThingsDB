#include <ti/fn/fn.h>


typedef int (*jload__set_cb) (void *, ti_val_t *, ex_t *);

static ti_raw_t * jload__key;

typedef struct
{
    size_t deep;
    ti_val_t * out;
    void * parents[YAJL_MAX_DEPTH];
    jload__set_cb callbacks[YAJL_MAX_DEPTH];
    ex_t * e;
    ti_collection_t * collection;
} jload__convert_t;

static int jload__root_cb(
        jload__convert_t * parent,
        ti_val_t * val,
        ex_t * UNUSED(e))
{
    parent->out = val;
    return 0;
}

static int jload__array_cb(ti_varr_t * parent, ti_val_t * val, ex_t * e)
{
    if (ti_val_varr_append(parent, &val, e))
        ti_val_unsafe_drop(val);
    return e->nr;
}

static int jload__map_cb(ti_thing_t * parent, ti_val_t * val, ex_t * e)
{
    if (ti_thing_o_set(parent, jload__key, val))
    {
        ex_set_mem(e);
        ti_val_unsafe_drop(val);
        return e->nr;
    }
    jload__key = NULL;
    return 0;
}

static int jload__null(void * ctx)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = (ti_val_t *) ti_nil_get();
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static int jload__boolean(void * ctx, int boolean)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = (ti_val_t *) ti_vbool_get(boolean);
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static int jload__integer(void * ctx, long long i)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = (ti_val_t *) ti_vint_create(i);
    if (!val)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static int jload__double(void * ctx, double d)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = (ti_val_t *) ti_vfloat_create(d);
    if (!val)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static int jload__string(void * ctx, const unsigned char * s, size_t n)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = (ti_val_t *) ti_str_create((const char * ) s, n);
    if (!val)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static int jload__start_map(void * ctx)
{
    jload__convert_t * c = (jload__convert_t *) ctx;

    ti_val_t * val = (ti_val_t *) ti_thing_o_create(0, 7, c->collection);
    if (!val)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }

    if (c->callbacks[c->deep](c->parents[c->deep], val, c->e))
        return 0;  /* failed */

    if (++c->deep >= YAJL_MAX_DEPTH)
    {
        ex_set(c->e, EX_OPERATION, "JSON max depth exceeded");
        return 0;  /* error */
    }

    c->parents[c->deep] = val;
    c->callbacks[c->deep] = (jload__set_cb) jload__map_cb;
    return 1;  /* success */
}

static int jload__map_key(void * ctx, const unsigned char * s, size_t n)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    jload__key = ti_str_create((const char *) s, n);
    if (!jload__key)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }
    return 1;  /* success */
}

static int jload__end_map(void * ctx)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    --c->deep;
    return 1;  /* success */
}

static int jload__start_array(void * ctx)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val;

    if (++c->deep >= YAJL_MAX_DEPTH)
    {
        ex_set(c->e, EX_OPERATION, "JSON max depth exceeded");
        return 0;  /* error */
    }

    val = (ti_val_t *) ti_varr_create(7);
    if (!val)
    {
        ex_set_mem(c->e);
        return 0;  /* failed */
    }

    c->parents[c->deep] = val;
    c->callbacks[c->deep] = (jload__set_cb) jload__array_cb;
    return 1;  /* success */
}

static int jload__end_array(void * ctx)
{
    jload__convert_t * c = (jload__convert_t *) ctx;
    ti_val_t * val = c->parents[c->deep--];
    return 0 == c->callbacks[c->deep](c->parents[c->deep], val, c->e);
}

static yajl_callbacks jload__callbacks = {
    jload__null,
    jload__boolean,
    jload__integer,
    jload__double,
    NULL,
    jload__string,
    jload__start_map,
    jload__map_key,
    jload__end_map,
    jload__start_array,
    jload__end_array
};

static int do__f_json_load(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    yajl_handle hand;
    yajl_status stat = yajl_status_error;
    jload__convert_t ctx = {0};
    ti_raw_t * raw;

    ctx.collection = query->collection;
    ctx.e = e;
    ctx.callbacks[0] = (jload__set_cb) jload__root_cb;
    ctx.parents[0] = &ctx;

    if (fn_nargs("json_load", DOC_JSON_LOAD, 1, nargs, e) ||
        ti_do_statement(query, nd->children, e) ||
        fn_arg_str("json_load", DOC_JSON_LOAD, 1, query->rval, e))
        return e->nr;

    hand = yajl_alloc(&jload__callbacks, NULL, &ctx);
    if (!hand)
    {
        ex_set_mem(e);
        return e->nr;
    }

    raw = (ti_raw_t *) query->rval;
    stat = yajl_parse(hand, raw->data, raw->n);

    if (stat == yajl_status_ok)
    {
        ti_val_unsafe_drop(query->rval);
        query->rval = ctx.out ? ctx.out : (ti_val_t *) ti_nil_get();
    }
    else
    {
        if (e->nr == 0)
        {
            static const int verbose = 0;
            unsigned char * yerr = yajl_get_error(
                    hand,
                    verbose,
                    raw->data,
                    raw->n);
            if (yerr)
            {
                ex_set(e, EX_VALUE_ERROR, (const char *) yerr);
                yajl_free_error(hand, yerr);
            }
            else
                ex_set_mem(e);
        }
        while (ctx.deep)
        {
            /*
             * Each but the last parent is a value type. If an array, then the
             * value exists but is not appended to another type. Thus, we need
             * to clear the array's in the list.
             */
            ti_val_t * val = ctx.parents[ctx.deep--];
            if (val && ti_val_is_array(val))
                ti_val_unsafe_drop(val);
        }

        ti_val_drop(ctx.out);
        free(jload__key);
        jload__key = NULL;
    }

    yajl_free(hand);
    return e->nr;
}
