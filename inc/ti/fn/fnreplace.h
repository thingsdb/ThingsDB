#include <ti/fn/fn.h>

/* Replace string with string num (reverse) times */
ti_raw_t * replacessr(
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

/* Replace string with string n times */
ti_raw_t * replacessn(
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

/* Replace string using closure n times */
ti_raw_t * replacescr(
        ti_raw_t * vstr,
        ti_raw_t * vold,
        ti_closure_t * closure,
        size_t n,
        ti_query_t * query,
        ex_t * e)
{
    ti_raw_t * raw = NULL;
    size_t strn = vstr->n;
    size_t oldn = vold->n;
    char * s = (char *) vstr->data ;
    char * pt = s + strn;
    char * old = (char *) vold->data;
    char * start = (char *) vstr->data;
    char * until = start + oldn;
    rbuf_t buf;
    rbuf_init(&buf);

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail0;

    do
    {
        if (n && pt >= until && memcmp(pt-oldn, old, oldn) == 0)
        {
            ti_raw_t * new;

            if (ti_closure_vars_replace_str(closure, pt - s, vold->n, vstr))
            {
                ex_set_mem(e);
                goto fail1;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail1;

            if (!ti_val_is_str(query->rval))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "replace callback is expecting a return value of "
                        "type `"TI_VAL_STR_S"` but got type `%s` instead"
                        DOC_STR_REPLACE,
                        ti_val_str(query->rval));
                goto fail1;
            }

            new = (ti_raw_t *) query->rval;


            if (rbuf_append(&buf, (const char *) new->data, new->n))
            {
                ex_set_mem(e);
                goto fail1;
            }
            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;

            pt -= oldn;

            --n;
            continue;
        }
        --pt;
        if (rbuf_write(&buf, *pt))
        {
            ex_set_mem(e);
            goto fail1;
        }
    }
    while (pt > start);

    raw = ti_str_create(buf.data + buf.pos, rbuf_len(&buf));
    if (!raw)
        ex_set_mem(e);
    query->rval = (ti_val_t *) raw;

fail1:
    ti_closure_dec(closure, query);
    free(buf.data);
fail0:
    return raw;

}

/* Replace string using closure n times */
int replacescn(
        ti_raw_t * vstr,
        ti_raw_t * vold,
        ti_closure_t * closure,
        ti_vint_t * vnum,
        ti_query_t * query,
        ex_t * e)
{
    assert (!vnum || vnum->int_ > 0);
    assert (vold->n && vold->n <= vstr->n);

    ti_raw_t * raw;
    size_t strn = vstr->n;
    size_t oldn = vold->n;
    size_t n = vnum ? (size_t) vnum->int_: SIZE_MAX;
    char * s = (char *) vstr->data;
    char * pt = s;
    char * end = pt + strn;
    char * old = (char *) vold->data;

    char * until = end - oldn;
    buf_t buf;
    buf_init(&buf);

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail0;

    while(pt < end)
    {
        if (n && pt <= until && memcmp(pt, old, oldn) == 0)
        {
            ti_raw_t * new;

            if (ti_closure_vars_replace_str(closure, pt - s, vold->n, vstr))
            {
                ex_set_mem(e);
                goto fail1;
            }

            if (ti_closure_do_statement(closure, query, e))
                goto fail1;

            if (!ti_val_is_str(query->rval))
            {
                ex_set(e, EX_TYPE_ERROR,
                        "replace callback is expecting a return value of "
                        "type `"TI_VAL_STR_S"` but got type `%s` instead"
                        DOC_STR_REPLACE,
                        ti_val_str(query->rval));
                goto fail1;
            }

            new = (ti_raw_t *) query->rval;

            if (buf_append(&buf, (const char *) new->data, new->n))
            {
                ex_set_mem(e);
                goto fail1;
            }

            ti_val_unsafe_drop(query->rval);
            query->rval = NULL;

            pt += oldn;
            --n;
            continue;
        }
        if (buf_write(&buf, *pt))
        {
            ex_set_mem(e);
            goto fail1;
        }
        ++pt;
    }

    raw = ti_str_create(buf.data, buf.len);
    if (!raw)
        ex_set_mem(e);
    query->rval = (ti_val_t *) raw;

fail1:
    ti_closure_dec(closure, query);
    free(buf.data);
fail0:
    return e->nr;
}

/* Replace string using closure n times */
int replacersn(
        ti_raw_t * vstr,
        ti_regex_t * regex,
        ti_raw_t * vnew,
        ti_vint_t * vnum,
        ti_query_t * query,
        ex_t * e)
{
    assert (!vnum || vnum->int_ > 0);

    ti_raw_t * raw;
    size_t n = vnum ? (size_t) vnum->int_: SIZE_MAX;
    size_t pos = 0;
    int rc;
    const char * s = (const char *) vstr->data;

    buf_t buf;
    buf_init(&buf);

    while (n-- && (rc = pcre2_match(
            regex->code,
            (PCRE2_SPTR8) vstr->data,
            vstr->n,
            pos,                   /* start looking at this point */
            0,                     /* OPTIONS */
            regex->match_data,
            NULL)) >= 0)
    {
       PCRE2_SIZE * ovector = pcre2_get_ovector_pointer(regex->match_data);

       if (buf_append(&buf, s + pos, ovector[0] - pos) ||
           buf_append(&buf, (const char *) vnew->data, vnew->n))
       {
           ex_set_mem(e);
           goto fail0;
       }

       if (pos == ovector[1])
           break;
       pos = ovector[1];
    }

    if (buf_append(&buf, s + pos, vstr->n - pos))
        goto fail0;

    raw = ti_str_create(buf.data, buf.len);
    if (!raw)
        ex_set_mem(e);
    query->rval = (ti_val_t *) raw;

fail0:
    free(buf.data);
    return e->nr;
}

/* Replace string using closure n times */
int replacercn(
        ti_raw_t * vstr,
        ti_regex_t * regex,
        ti_closure_t * closure,
        ti_vint_t * vnum,
        ti_query_t * query,
        ex_t * e)
{
    assert (!vnum || vnum->int_ > 0);

    ti_raw_t * raw;
    size_t n = vnum ? (size_t) vnum->int_: SIZE_MAX;
    size_t pos = 0;
    int rc;
    const char * s = (const char *) vstr->data;

    buf_t buf;
    buf_init(&buf);

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        goto fail0;

    while (n-- && (rc = pcre2_match(
            regex->code,
            (PCRE2_SPTR8) vstr->data,
            vstr->n,
            pos,                   /* start looking at this point */
            0,                     /* OPTIONS */
            regex->match_data,
            NULL)) >= 0)
    {
        ti_raw_t * new;
        PCRE2_SIZE * ovector = pcre2_get_ovector_pointer(regex->match_data);

        if (buf_append(&buf, s + pos, ovector[0] - pos) ||
            ti_closure_vars_replace_regex(closure, vstr, ovector, rc))
        {
            ex_set_mem(e);
            goto fail1;
        }

        if (ti_closure_do_statement(closure, query, e))
            goto fail1;

        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "replace callback is expecting a return value of "
                    "type `"TI_VAL_STR_S"` but got type `%s` instead"
                    DOC_STR_REPLACE,
                    ti_val_str(query->rval));
            goto fail1;
        }

        new = (ti_raw_t *) query->rval;

        if (buf_append(&buf, (const char *) new->data, new->n))
        {
            ex_set_mem(e);
            goto fail1;
        }

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;

        if (pos == ovector[1])
            break;
        pos = ovector[1];
    }

    if (buf_append(&buf, s + pos, vstr->n - pos))
        goto fail0;

    raw = ti_str_create(buf.data, buf.len);
    if (!raw)
        ex_set_mem(e);
    query->rval = (ti_val_t *) raw;

fail1:
    ti_closure_dec(closure, query);
    free(buf.data);
fail0:
    return e->nr;
}

static int do__replace_str(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    ti_raw_t * str;
    ti_val_t * old, * new;
    ti_vint_t * vnum = NULL;
    cleri_children_t * child;

    if (fn_nargs_range("replace", DOC_STR_REPLACE, 2, 3, nargs, e))
        return e->nr;

    str = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = nd->children)->node, e) ||
        fn_arg_str_regex("replace", DOC_STR_REPLACE, 1, query->rval, e))
        goto fail0;

    old = query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_str_closure("replace", DOC_STR_REPLACE, 2, query->rval, e))
        goto fail1;

    new = query->rval;
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
            goto done;
        }

        if (vnum->int_ == 0)
        {
            query->rval = (ti_val_t *) str;
            ti_incref(str);
            goto done;
        }
    }

    /* vnum is from here on either `NULL` or an integer != zero */

    if (ti_val_is_str(old))
    {
        ti_raw_t * sold = (ti_raw_t *) old;

        if (sold->n == 0)
        {
            ex_set(e, EX_VALUE_ERROR, "replace an empty string");
            goto done;
        }

        if (sold->n > str->n)
        {
            query->rval = (ti_val_t *) str;
            ti_incref(str);
            goto done;
        }

        if(ti_val_is_str(new))
        {
            ti_raw_t * res = (!vnum || vnum->int_ > 0)
                ? replacessn(str, sold, (ti_raw_t *) new, vnum)
                : replacessr(str, sold, (ti_raw_t *) new, llabs(vnum->int_));
            if (!res)
                ex_set_mem(e);

            query->rval = (ti_val_t *) res;
            goto done;
        }

        /* `new` is of `ti_closure_t` */
        if (!vnum || vnum->int_ > 0)
            replacescn(
                    str,
                    sold,
                    (ti_closure_t *) new,
                    vnum,
                    query,
                    e);
        else
            replacescr(
                    str,
                    sold,
                    (ti_closure_t *) new,
                    llabs(vnum->int_),
                    query,
                    e);
        goto done;
    }

    if (vnum && vnum->int_ < 0)
    {
        ex_set(e, EX_VALUE_ERROR,
                    "function `replace` does not support backward (negative) "
                    "replacements when used with a regular expression"
                    DOC_STR_REPLACE);
        goto done;
    }

    /* `old` is of `ti_regex_t` */
    if(ti_val_is_str(new))
        replacersn(
                str,
                (ti_regex_t *) old,
                (ti_raw_t *) new,
                vnum,
                query,
                e);
    else
        replacercn(
                str,
                (ti_regex_t *) old,
                (ti_closure_t *) new,
                vnum,
                query,
                e);

done:
    ti_val_drop((ti_val_t *) vnum);  /* might be NULL */
fail2:
    ti_val_unsafe_drop(new);
fail1:
    ti_val_unsafe_drop(old);
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
        ti_raw_t * key,
        ti_val_t * val,
        ex_t * e)
{
    int64_t i;

    switch(key->n)
    {
    case 3:
        if (memcmp(key->data, "day", 3) == 0)
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
        if (memcmp(key->data, "hour", 4) == 0)
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
        if (memcmp(key->data, "year", 4) == 0)
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
        if (memcmp(key->data, "month", 5) == 0)
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
        if (memcmp(key->data, "second", 6) == 0)
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
        if (memcmp(key->data, "minute", 6) == 0)
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

typedef struct
{
    struct tm * tm;
    ex_t * e;
} replace__walk_i_t;

static int replace__walk_i(ti_item_t * item, replace__walk_i_t * w)
{
    return do__replace_value(w->tm, item->key, item->val, w->e);
}

static int do__replace_datetime(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
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
        if (ti_thing_is_dict(thing))
        {
            replace__walk_i_t w = {
                    .e = e,
                    .tm = &tm,
            };
            if (smap_values(
                    thing->items.smap,
                    (smap_val_cb) replace__walk_i,
                    &w))
                return e->nr;
        }
        else
        {
            for (vec_each(thing->items.vec, ti_prop_t, p))
                if (do__replace_value(&tm, (ti_raw_t *) p->name, p->val, e))
                    return e->nr;
        }
    }
    else
    {
        ti_name_t * name;
        ti_val_t * val;
        for (thing_t_each(thing, name, val))
            if (do__replace_value(&tm, (ti_raw_t *) name, val, e))
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
