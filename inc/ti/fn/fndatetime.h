#include <ti/fn/fn.h>


static int fn__datetime(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    struct tm tm;
    cleri_children_t * child = nd->children;
    int64_t i;
    time_t ts;
    ti_datetime_t * dt;
    int mday;  /* saved for parse check */

    assert (langdef_nd_n_function_params(nd) >= 3);

    memset(&tm, 0, sizeof(struct tm));

    /*
     * Read year
     */

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 1 to be of "
                "type `"TI_VAL_INT_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    i = VINT(query->rval);
    if (i < 1 || i > 9999)
    {
        ex_set(e, EX_VALUE_ERROR,
                "year %"PRId64" is out of range [1..9999]"DOC_DATETIME,
                i);
        return e->nr;
    }

    tm.tm_year = (int) i - 1900;  /* tm_year = years since 1900 */

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /*
     * Read month
     */
    child = child->next->next;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 2 to be of "
                "type `"TI_VAL_INT_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    i = VINT(query->rval);
    if (i < 1 || i > 12)
    {
        ex_set(e, EX_VALUE_ERROR,
                "month %"PRId64" is out of range [1..12]"DOC_DATETIME,
                i);
        return e->nr;
    }

    tm.tm_mon = (int) i - 1;  /* tm_mon = months since January */

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /*
     * Read day
     */
    child = child->next->next;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    if (!ti_val_is_int(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 3 to be of "
                "type `"TI_VAL_INT_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    i = VINT(query->rval);
    if (i < 1 || i > 31)
    {
        ex_set(e, EX_VALUE_ERROR,
                "day %"PRId64" is out of range [1..31]"DOC_DATETIME,
                i);
        return e->nr;
    }

    mday = tm.tm_mday = (int) i;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    /*
     * Read (optional) hour
     */
    if (!child->next || !(child = child->next->next))
        goto done;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    switch ((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_INT:
        i = VINT(query->rval);
        if (i < 0 || i > 23)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "hour %"PRId64" is out of range [0..23]"DOC_DATETIME,
                    i);
            return e->nr;
        }

        tm.tm_hour = (int) i;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
        goto done;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 4 to be of "
                "type `"TI_VAL_INT_S"` or type `"TI_VAL_STR_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    /*
     * Read (optional) minute
     */
    if (!child->next || !(child = child->next->next))
        goto done;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    switch ((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_INT:
        i = VINT(query->rval);
        if (i < 0 || i > 59)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "minute %"PRId64" is out of range [0..59]"DOC_DATETIME,
                    i);
            return e->nr;
        }

        tm.tm_min = (int) i;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
        goto done;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 5 to be of "
                "type `"TI_VAL_INT_S"` or type `"TI_VAL_STR_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    /*
     * Read (optional) second
     */
    if (!child->next || !(child = child->next->next))
        goto done;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    switch ((ti_val_enum) query->rval->tp)
    {
    case TI_VAL_INT:
        i = VINT(query->rval);
        if (i < 0 || i > 59)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "second %"PRId64" is out of range [0..59]"DOC_DATETIME,
                    i);
            return e->nr;
        }

        tm.tm_sec = (int) i;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
        break;
    case TI_VAL_NAME:
    case TI_VAL_STR:
        goto done;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects argument 6 to be of "
                "type `"TI_VAL_INT_S"` or type `"TI_VAL_STR_S"` "
                "(when called with three or more arguments) "
                "but got type `%s` instead"DOC_DATETIME,
                ti_val_str(query->rval));
        return e->nr;
    }

    /*
     * Read (optional) timezone info
     */
    if (!child->next || !(child = child->next->next))
        goto done;

    if (ti_do_statement(query, child->node, e))
        return e->nr;

    if (!ti_val_is_str(query->rval))
    {
        ex_set(e, EX_TYPE_ERROR,
            "function `datetime` expects argument 7 to be of "
            "type `"TI_VAL_STR_S"` "
            "(when called with three or more arguments) "
            "but got type `%s` instead"DOC_DATETIME,
            ti_val_str(query->rval));
        return e->nr;
    }

done:
    ts = mktime(&tm);

    if (tm.tm_mday != mday)
    {
        ex_set(e, EX_VALUE_ERROR,
                "day %s is out of range for month"DOC_DATETIME,
                mday);
        return e->nr;
    }

    dt = ti_datetime_from_i64(ts, 0);
    if (!dt)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (query->rval)
    {
        if (child && child->next)
        {
            ex_set(e, EX_TYPE_ERROR,
                "function `datetime` expects the last, and only the last "
                "argument to be of type `"TI_VAL_STR_S"` "
                "(when called with three or more arguments)"DOC_DATETIME);
            return e->nr;
        }

        /* calculate time zone info */
        (void) (
            ti_datetime_to_zone(dt, (ti_raw_t *) query->rval, e) ||
            ti_datetime_localize(dt, e));

        ti_val_unsafe_drop(query->rval);
    }

    query->rval = (ti_val_t *) dt;  /* may be NULL */
    return e->nr;
}

static int do__f_datetime(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_nargs_max("datetime", DOC_DATETIME, 7, nargs, e))
        return e->nr;

    switch (nargs)
    {
    case 0:
        /* Return the current date/time. Equal to `datetime(now());` */
        assert (query->rval == NULL);
        query->rval = (ti_val_t *) ti_datetime_from_i64(
                (int64_t) util_now_tsec(),
                0);
        if (!query->rval)
            ex_set_mem(e);
        return e->nr;

    case 1:
        /*
         * An integer or float representing a time-stamp, a datetime type,
         * or a string representing a datetime.
         */
        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        switch ((ti_val_enum) query->rval->tp)
        {
        case TI_VAL_INT:
        {
            int64_t i = VINT(query->rval);
            ti_val_unsafe_drop(query->rval);
            query->rval = (ti_val_t *) ti_datetime_from_i64(i, 0);
            if (!query->rval)
                ex_set_mem(e);
            return e->nr;
        }
        case TI_VAL_FLOAT:
        {
            double d = VFLOAT(query->rval);
            int64_t i;
            if (ti_val_overflow_cast(d))
            {
                ex_set(e, EX_OVERFLOW, "datetime overflow");
                return e->nr;
            }
            i = (int64_t) d;
            ti_val_unsafe_drop(query->rval);
            query->rval = (ti_val_t *) ti_datetime_from_i64(i, 0);
            if (!query->rval)
                ex_set_mem(e);
            return e->nr;
        }
        case TI_VAL_DATETIME:
            return e->nr;
        case TI_VAL_NAME:
        case TI_VAL_STR:
        {
            ti_datetime_t * dt = ti_datetime_from_str(
                    (ti_raw_t *) query->rval,
                    e);
            if (dt)
            {
                ti_val_unsafe_drop(query->rval);
                query->rval = (ti_val_t *) dt;

            }
            return e->nr;
        }
        default:
            ex_set(e, EX_TYPE_ERROR,
                    "cannot convert type `%s` to `"TI_VAL_DATETIME_S"`"
                    DOC_DATETIME,
                    ti_val_str(query->rval));
        }
        return e->nr;
    case 2:
    {
        ti_raw_t * str, * fmt;
        /*
         * Accept datetime(string, format);
         */
        if (ti_do_statement(query, nd->children->node, e))
            return e->nr;

        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `datetime` expects argument 1 to be of "
                    "type `"TI_VAL_STR_S"` (when called using 2 arguments) "
                    "but got type `%s` instead"DOC_DATETIME,
                    ti_val_str(query->rval));
            return e->nr;
        }

        str = (ti_raw_t *) query->rval;
        query->rval = NULL;

        if (ti_do_statement(query, nd->children->next->next->node, e))
            goto fail0;

        if (!ti_val_is_str(query->rval))
        {
            ex_set(e, EX_TYPE_ERROR,
                    "function `datetime` expects argument 2 to be of "
                    "type `"TI_VAL_STR_S"` (when called with two arguments) "
                    "but got type `%s` instead"DOC_DATETIME,
                    ti_val_str(query->rval));
            return e->nr;
        }

        fmt = (ti_raw_t *) query->rval;
        query->rval = (ti_val_t *) ti_datetime_from_fmt(str, fmt, e);
        ti_val_unsafe_drop((ti_val_t *) fmt);
fail0:
        ti_val_unsafe_drop((ti_val_t *) str);
        return e->nr;
    }
    default:
        /*
         * Accept year, month, day... construction, with optimal time-zone info
         */
        return fn__datetime(query, nd, e);
    }
}
