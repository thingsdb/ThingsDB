#include <ti/fn/fn.h>

typedef struct
{
    uint8_t deep;
    int flags;
    ex_t * e;
} json_dumps__options_t;

static int json_dump__walk_cb(
        ti_raw_t * key,
        ti_val_t * val,
        json_dumps__options_t * options)
{
    if (key == (ti_raw_t *) ti_val_borrow_deep_name())
        return ti_deep_from_val(val, &options->deep, options->e);

    if (key == (ti_raw_t *) ti_val_borrow_beautify_name())
    {
        if (ti_val_as_bool(val))
            options->flags |= MPJSON_FLAG_BEAUTIFY;
        else
            options->flags &= ~MPJSON_FLAG_BEAUTIFY;
        return 0;
    }

    ex_set(options->e, EX_LOOKUP_ERROR,
            "invalid option `%.*s` for function `json_dump`"DOC_JSON_DUMP,
            key->n, key->data);
    return options->e->nr;
}

static int do__f_json_dump(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    msgpack_sbuffer buffer;
    yajl_gen_status stat;
    ti_raw_t * raw;
    ti_val_t * val;
    size_t total_n;
    ti_vp_t vp = {
            .query=query,
    };
    json_dumps__options_t options = {
            .deep = query->qbind->deep,
            .flags = 0,
            .e = e,
    };

    if (fn_nargs_range("json_dump", DOC_JSON_DUMP, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    val = query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_thing("json_dump", DOC_JSON_DUMP, 2, query->rval, e) ||
            ti_thing_walk(
                    (ti_thing_t *) query->rval,
                    (ti_thing_item_cb) json_dump__walk_cb,
                    &options))
            goto fail0;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    if (mp_sbuffer_alloc_init(&buffer, ti_val_alloc_size(val), 0))
    {
        ex_set_mem(e);
        goto fail0;
    }

    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    if (ti_val_to_client_pk(val, &vp, options.deep))
    {
        ex_set_mem(e);
        goto fail1;
    }

    stat = mpjson_mp_to_json(
                buffer.data,
                buffer.size,
                (unsigned char **) &raw,
                &total_n,
                sizeof(ti_raw_t),
                options.flags);
    if (stat)
    {
        mpjson__set_err(e, stat);
        goto fail1;
    }

    ti_raw_init(raw, TI_VAL_STR, total_n);
    query->rval = (ti_val_t *) raw;

fail1:
    msgpack_sbuffer_destroy(&buffer);
fail0:
    ti_val_unsafe_drop(val);
    return e->nr;
}
