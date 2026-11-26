#include <ti/fn/fn.h>

static int do__f_encrypt(ti_query_t *query, cleri_node_t *nd, ex_t *e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t *raw, *key;
    unsigned char * enc;
    size_t enc_n;

    if (!ti_val_is_bytes(query->rval))
        return fn_call_try("encrypt", query, nd, e);

    if (fn_nargs("encrypt", DOC_BYTES_ENCRYPT, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str_bytes("encrypt", DOC_BYTES_ENCRYPT, 1, query->rval, e))
        goto fail0;

    key = (ti_raw_t *) query->rval;

    if (key->n == 0)
    {
        ex_set(e, EX_VALUE_ERROR, "key must not be empty");
        goto fail0;
    }
    if (key->n >= INT_MAX)
    {
        ex_set(e, EX_OVERFLOW, "key too long");
        goto fail0;
    }
    if (raw->n >= INT_MAX - CRYPTX_HDR_LEN)
    {
        ex_set(e, EX_OVERFLOW, "content too large to encrypt");
        goto fail0;
    }
    if (cryptx_encrypt(raw->data, raw->n, key->data, key->n, &enc, &enc_n))
    {
        ex_set_mem(e);
        goto fail0;
    }

    query->rval = (ti_val_t *) ti_bin_create(enc, enc_n);
    if (!query->rval)
        ex_set_mem(e);

    free(enc);
    ti_val_unsafe_drop((ti_val_t *) key);
fail0:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
