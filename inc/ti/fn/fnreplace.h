#include <ti/fn/fn.h>

ti_raw_t * replacer(
        ti_raw_t * vstr,
        ti_raw_t * vold,
        ti_raw_t * vnew,
        size_t num)
{
    assert (vold->n && vold->n <= vstr->n);
    assert (num);

    ti_raw_t * raw;
    size_t strn = vstr->n;
    size_t oldn = vold->n;
    size_t newn = vnew->n;
    size_t mo = strn/oldn;
    size_t n = num < mo ? num : mo;
    size_t sz = newn <= oldn ? strn : n*newn + strn;
    char * pt = (char *) vstr->data + strn;
    char * old = (char *) vold->data;
    char * start = (char *) vstr->data;
    char * until = start + oldn;
    char * tmp = malloc(sz);
    char * wt = tmp + sz;
    if (!tmp)
        return NULL;

    do
    {
        if (n && pt >= until && memcmp(pt-oldn, old, oldn) == 0)
        {
            wt -= newn;
            pt -= oldn;
            memcpy(wt, vnew->data, newn);
            --n;
            continue;
        }
        --wt;
        --pt;
        *wt = *pt;
    }
    while (pt > start);

    raw = ti_str_create(wt, (tmp + sz)-wt);
    free(tmp);
    return raw;
}

ti_raw_t * replacen(
        ti_raw_t * vstr,
        ti_raw_t * vold,
        ti_raw_t * vnew,
        ti_vint_t * vnum)
{
    assert (!vnum || vnum->int_ > 0);
    assert (vold->n && vold->n <= vstr->n);

    ti_raw_t * raw;
    size_t strn = vstr->n;
    size_t oldn = vold->n;
    size_t newn = vnew->n;
    size_t mo = strn/oldn;
    size_t n = vnum && (size_t) vnum->int_ < mo ? (size_t) vnum->int_: mo;
    size_t sz = newn <= oldn ? strn : n*newn + strn;
    char * pt = (char *) vstr->data;
    char * old = (char *) vold->data;
    char * end = pt + strn;
    char * until = end - oldn;
    char * tmp = malloc(sz);
    char * wt = tmp;
    if (!tmp)
        return NULL;

    while(pt < end)
    {
        if (n && pt <= until && memcmp(pt, old, oldn) == 0)
        {
            memcpy(wt, vnew->data, newn);
            wt += newn;
            pt += oldn;
            --n;
            continue;
        }
        *wt = *pt;
        ++pt;
        ++wt;
    }

    raw = ti_str_create(tmp, wt-tmp);
    free(tmp);
    return raw;
}

static int do__f_replace(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * str, * sold, * snew, * res;
    ti_vint_t * vnum = NULL;
    cleri_children_t * child;

    if (!ti_val_is_str(query->rval))
        return fn_call_try("replace", query, nd, e);

    if (fn_nargs_range("replace", DOC_STR_REPLACE, 2, 3, nargs, e))
        return e->nr;

    str = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = nd->children)->node, e) ||
        fn_arg_str("replace", DOC_STR_REPLACE, 1, query->rval, e))
        goto fail0;

    sold = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (sold->n == 0)
    {
        ex_set(e, EX_VALUE_ERROR, "replace an empty string");
            goto fail1;
    }

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_str("replace", DOC_STR_REPLACE, 2, query->rval, e))
        goto fail1;

    snew = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (nargs == 3)
    {
        if (ti_do_statement(query, (child = child->next->next)->node, e) ||
            fn_arg_int("replace", DOC_STR_REPLACE, 2, query->rval, e))
            goto fail2;

        vnum = (ti_vint_t *) query->rval;
        query->rval = NULL;

        if (vnum->int_ == LLONG_MIN)
        {
            ex_set(e, EX_OVERFLOW, "integer overflow");
            goto fail3;
        }
    }

    if (sold->n > str->n || (vnum && vnum->int_ == 0))
    {
        res = str;
        ti_incref(res);
    }
    else
    {
        res = (!vnum || vnum->int_ > 0)
            ? replacen(str, sold, snew, vnum)
            : replacer(str, sold, snew, llabs(vnum->int_));
    }

    query->rval = (ti_val_t *) res;
    if (!res)
        ex_set_mem(e);

fail3:
    ti_val_drop((ti_val_t *) vnum);
fail2:
    ti_val_unsafe_drop((ti_val_t *) snew);
fail1:
    ti_val_unsafe_drop((ti_val_t *) sold);
fail0:
    ti_val_unsafe_drop((ti_val_t *) str);
    return e->nr;
}
