#include <ti/fn/fn.h>

static int do__f_decrypt(ti_query_t *query, cleri_node_t *nd, ex_t *e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t *raw, *key;
    unsigned char * dec;
    size_t dec_n;
    int rc;

    if (!ti_val_is_bytes(query->rval))
        return fn_call_try("decrypt", query, nd, e);

    if (fn_nargs("decrypt", DOC_BYTES_DECRYPT, 1, nargs, e))
        return e->nr;

    raw = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_str_bytes("decrypt", DOC_BYTES_DECRYPT, 1, query->rval, e))
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
        ex_set(e, EX_OVERFLOW, "content too large to decrypt");
        goto fail0;
    }
    rc = cryptx_decrypt(raw->data, raw->n, key->data, key->n, &dec, &dec_n);
    if (rc != 0)
    {
        if (rc == CRYPTX_ERROR_MEM)
            ex_set_mem(e);
        else
            ex_set(e, EX_BAD_DATA, "decryption failed: invalid data or key");
        goto fail0;
    }

    query->rval = (ti_val_t *) ti_bin_create(dec, dec_n);
    if (!query->rval)
        ex_set_mem(e);

    free(dec);
    ti_val_unsafe_drop((ti_val_t *) key);
fail0:
    ti_val_drop((ti_val_t *) raw);
    return e->nr;
}
