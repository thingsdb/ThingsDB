#include <ti/fn/fn.h>


static int do__f_match(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_regex_t * regex;
    ti_raw_t * vstr;
    ti_varr_t * arr;
    int rc, i;
    PCRE2_SIZE * ovector;

    if (!ti_val_is_regex(query->rval))
        return fn_call_try("match", query, nd, e);

    if (fn_nargs("match", DOC_REGEX_MATCH, 1, nargs, e))
        return e->nr;

    regex = (ti_regex_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e))
        goto fail0;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `match` expects argument 1 to be "
            "of type `"TI_VAL_STR_S"` but got type `%s` instead"DOC_REGEX_MATCH,
            ti_val_str(query->rval));
        goto fail0;
    }

    vstr = (ti_raw_t *) query->rval;
    query->rval = NULL;

    /* TODO: we could allow `g` as regular expression flag which in turn could
     *       change the behavior for match(..) to return all matches similar
     *       to javascripts match.
     */
    rc = pcre2_match(
            regex->code,
            (PCRE2_SPTR8) vstr->data,
            vstr->n,
            0,                     /* start looking at this point */
            0,                     /* OPTIONS */
            regex->match_data,
            NULL);

    if (rc < 0)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        goto fail1;  /* success, but no match */
    }
    ovector = pcre2_get_ovector_pointer(regex->match_data);

    arr = ti_varr_create(rc);
    if (!arr)
    {
        ex_set_mem(e);
        goto fail1;
    }
    query->rval = (ti_val_t *) arr;

    for (i = 0; i < rc; i++)
    {
        ti_raw_t * substr;
        PCRE2_SPTR pt = vstr->data + ovector[2*i];
        PCRE2_SIZE n =  ovector[2*i+1] - ovector[2*i];

        substr = ti_str_create((const char *) pt, n);
        if (!substr)
        {
            ex_set_mem(e);
            goto fail1;
        }

        VEC_push(arr->vec, substr);
    }

fail1:
    ti_val_unsafe_drop((ti_val_t *) vstr);
fail0:
    ti_val_unsafe_drop((ti_val_t *) regex);
    return e->nr;
}
