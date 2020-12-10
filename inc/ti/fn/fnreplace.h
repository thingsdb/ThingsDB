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

static int do__replace_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    ti_raw_t * str, * sold, * snew, * res;
    ti_vint_t * vnum = NULL;
    cleri_children_t * child;

    if (fn_nargs_range("replace", DOC_STR_REPLACE, 2, 3, nargs, e))
        return e->nr;

    str = (ti_raw_t *) query->rval;
    query->rval = NULL;

    /*
     * TODO: It would be a nice feature if the `old` argument could also be
     *       a regular expression, instead of only a type string.
     */
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



/*
 * replaces the same values as exported by `as_thing()`, except for gmt_offset:
 *      - year
 *      - month
 *      - day
 *      - hour
 *      - minute
 *      - second
 */
static int do__replace_value(
        struct tm * tm,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    int64_t i;

    switch(name->n)
    {
    case 3:
        if (memcmp(name->str, "day", 3) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "day must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 1 || i > 31)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "day %"PRId64" is out of range [1..31]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }
            tm->tm_mday = (int) i;
        }
        return 0;
    case 4:
        if (memcmp(name->str, "hour", 4) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "hour must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 0 || i > 23)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "hour %"PRId64" is out of range [0..23]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }
            tm->tm_hour = (int) i;
            return 0;
        }
        if (memcmp(name->str, "year", 4) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "year must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 1 || i > 9999)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "year %"PRId64" is out of range [1..9999]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }
            tm->tm_year = (int) i - 1900;  /* tm_year = years since 1900 */
        }
        return 0;
    case 5:
        if (memcmp(name->str, "month", 5) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "month must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 1 || i > 12)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "month %"PRId64" is out of range [1..12]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }

            tm->tm_mon = (int) i - 1;  /* tm_mon = months since January */
        }
        return 0;
    case 6:
        if (memcmp(name->str, "second", 6) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "second must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 0 || i > 59)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "second %"PRId64" is out of range [0..59]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }

            tm->tm_sec = (int) i;
            return 0;
        }
        if (memcmp(name->str, "minute", 6) == 0)
        {
            if (!ti_val_is_int(val))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "minute must be of type `"TI_VAL_INT_S"` but "
                        "got type `%s` instead"DOC_DATETIME_REPLACE,
                        ti_val_str(val));
                return e->nr;
            }

            i = VINT(val);
            if (i < 0 || i > 59)
            {
                ex_set(e, EX_VALUE_ERROR,
                        "minute %"PRId64" is out of range [0..59]"
                        DOC_DATETIME_REPLACE,
                        i);
                return e->nr;
            }

            tm->tm_min = (int) i;
        }
        return 0;
    }
    return 0;
}

static int do__replace_datetime(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    uint8_t flags;
    ti_datetime_t * dt;
    ti_tz_t * tz;
    ti_thing_t * thing;
    struct tm tm;

    if (fn_nargs("replace", DOC_DATETIME_REPLACE, 1, nargs, e))
        return e->nr;

    dt = (ti_datetime_t *) query->rval;
    tz = dt->tz;
    flags = dt->flags;

    if (ti_datetime_time(dt, &tm))
    {
        ex_set(e, EX_VALUE_ERROR, "failed to localize time");
        return e->nr;
    }

    ti_val_unsafe_drop((ti_val_t *) dt);
    query->rval = NULL;

    if (ti_do_statement(query, nd->children->node, e) ||
        fn_arg_thing("replace", DOC_DATETIME_REPLACE, 1, query->rval, e))
        return e->nr;

    thing = (ti_thing_t *) query->rval;

    if (ti_thing_is_object(thing))
    {
        for (vec_each(thing->items, ti_prop_t, p))
            if (do__replace_value(&tm, p->name, p->val, e))
                return e->nr;
    }
    else
    {
        ti_name_t * name;
        ti_val_t * val;
        for (thing_t_each(thing, name, val))
            if (do__replace_value(&tm, name, val, e))
                return e->nr;
    }

    ti_val_unsafe_drop((ti_val_t *) thing);
    query->rval = (ti_val_t *) ti_datetime_from_tm_tz(&tm, tz, e);

    if (query->rval)
        query->rval->flags = flags;  /* restore datetime flags */

    return e->nr;
}


static inline int do__f_replace(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    return ti_val_is_str(query->rval)
            ? do__replace_str(query, nd, e)
            : ti_val_is_datetime(query->rval)
            ? do__replace_datetime(query, nd, e)
            : fn_call_try("replace", query, nd, e);
}
