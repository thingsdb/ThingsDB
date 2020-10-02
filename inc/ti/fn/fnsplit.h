#include <ti/fn/fn.h>

typedef size_t (*split__cmp) (char *, char *, size_t);

size_t split__memcmp(char * a, char * b, size_t n)
{
    return memcmp(a, b, n) == 0 ? n : 0;
}

size_t split__whitespace(char * a, char * end, size_t UNUSED(_n))
{
    size_t n;
    for (n = 0; a != end && isspace(*a); ++n, ++a);
    return n;
}

size_t split__whitespace_r(char * a, char * end, size_t UNUSED(_n))
{
    size_t n;
    for (n = 0; a >= end && isspace(*a); ++n, --a);
    return n;
}

ti_varr_t * splitr(ti_raw_t * vstr, ti_raw_t * vsep, size_t num_splits)
{
    assert (!vsep || vsep->n);
    assert (num_splits);

    ti_varr_t * varr;
    ti_raw_t * raw;
    size_t n = vsep ? vsep->n : 1;
    size_t num = vstr->n / n;
    split__cmp cmp_cb = vsep ? split__memcmp : split__whitespace_r;
    char * pt = (char *) vstr->data+vstr->n;
    char * until = pt;
    char * stop = (char *) vstr->data + n;
    char * hlp = vsep ? (char *) vsep->data : (char *) vstr->data;

    num = num_splits < num ? num_splits : num;
    varr = ti_varr_create(num > 15 ? 15 : num + 1);
    if (!varr)
        return NULL;

    while(pt >= stop && num)
    {
        size_t skipr = cmp_cb(pt-n, hlp, n);
        if (skipr)
        {
            raw = pt == until
                    ? (ti_raw_t *) ti_val_empty_str()
                    : ti_str_create(pt, until - pt);
            if (!raw || vec_push(&varr->vec, raw))
                goto fail;
            --num;
            pt -= skipr;
            until = pt;
            continue;
        }
        --pt;
    }

    n = until - (char *) vstr->data;
    raw = n == 0
            ? (ti_raw_t *) ti_val_empty_str()
            : until == (char *) vstr->data+vstr->n
            ? ti_grab(vstr)
            : ti_str_create((char *) vstr->data, n);

    if (!raw || vec_push(&varr->vec, raw))
        goto fail;

    vec_reverse(varr->vec);
    (void) vec_shrink(&varr->vec);

    return varr;

fail:
    ti_varr_destroy(varr);
    return NULL;
}

ti_varr_t * splitn(ti_raw_t * vstr, ti_raw_t * vsep, ti_vint_t * vnum)
{
    assert (!vsep || vsep->n);
    assert (!vnum || vnum->int_ >= 0);

    ti_varr_t * varr;
    ti_raw_t * raw;
    size_t n = vsep ? vsep->n : 1;
    int64_t num = vstr->n / n;
    split__cmp cmp_cb = vsep ? split__memcmp : split__whitespace;
    char * pt = (char *) vstr->data;
    char * start = pt;
    char * end = n <= vstr->n ? pt + vstr->n - n : pt;
    char * hlp = vsep ? (char *) vsep->data : (char *) vstr->data+vstr->n;

    num = vnum && vnum->int_ < num ? vnum->int_ : num;
    varr = ti_varr_create(num > 15 ? 15 : num + 1);
    if (!varr)
        return NULL;

    while(pt <= end && num)
    {
        size_t skipn = cmp_cb(pt, hlp, n);
        if (skipn)
        {
            raw = pt == start
                    ? (ti_raw_t *) ti_val_empty_str()
                    : ti_str_create(start, pt - start);
            if (!raw || vec_push(&varr->vec, raw))
                goto fail;

            --num;
            pt += skipn;
            start = pt;
            continue;
        }
        ++pt;
    }

    n = ((char *) vstr->data + vstr->n) - start;
    raw = n == 0
            ? (ti_raw_t *) ti_val_empty_str()
            : start == (char *) vstr->data
            ? ti_grab(vstr)
            : ti_str_create(start, n);

    if (!raw || vec_push(&varr->vec, raw))
        goto fail;

    (void) vec_shrink(&varr->vec);

    return varr;

fail:
    ti_varr_destroy(varr);
    return NULL;
}

static int do__f_split(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * str, * sep = NULL;
    ti_vint_t * vnum = NULL;
    ti_varr_t * varr;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("split", query, nd, e);

    if (fn_nargs_max("split", DOC_STR_SPLIT, 2, nargs, e))
        return e->nr;

    str = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs >= 1)
    {
        if (ti_do_statement(query, nd->children->node, e))
            goto fail;

        if (ti_val_is_str(query->rval))
        {
            sep = (ti_raw_t *) query->rval;
            query->rval = NULL;
            if (sep->n == 0)
            {
                ex_set(e, EX_VALUE_ERROR, "empty separator");
                goto fail;
            }
            if (nargs == 2)
            {
                if (ti_do_statement(query, nd->children->next->next->node, e) ||
                    fn_arg_int("split", DOC_STR_SPLIT, 2, query->rval, e))
                    goto fail;
            }
        }
        else if (ti_val_is_int(query->rval))
        {
            if (nargs == 2)
            {
                ex_set(e, EX_NUM_ARGUMENTS,
                    "function `split` takes at most 1 argument when the first "
                    "argument is of type `"TI_VAL_INT_S"`"DOC_STR_SPLIT);
                goto fail;
            }
        }
        else
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `split` expects argument 1 to be of "
                "type `"TI_VAL_STR_S"` or type `"TI_VAL_INT_S"` "
                "but got type `%s` instead"DOC_STR_SPLIT,
                ti_val_str(query->rval));
            goto fail;
        }

        vnum = (ti_vint_t *) query->rval;
        if (vnum)
        {
            query->rval = NULL;
            if (vnum->int_ == LLONG_MIN)
            {
                ex_set(e, EX_OVERFLOW, "integer overflow");
                goto fail;
            }
        }
    }

    varr = (!vnum || vnum->int_ >= 0)
        ? splitn(str, sep, vnum)
        : splitr(str, sep, llabs(vnum->int_));

    query->rval = (ti_val_t *) varr;
    if (!varr)
        ex_set_mem(e);

fail:
    ti_val_drop((ti_val_t *) vnum);
    ti_val_drop((ti_val_t *) sep);
    ti_val_unsafe_drop((ti_val_t *) str);
    return e->nr;
}
