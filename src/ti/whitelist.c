/*
 * ti/whitelist.c
 */
#include <ti.h>
#include <ti/opr.h>
#include <ti/regex.t.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/whitelist.h>
#include <util/logger.h>
#include <util/strx.h>


static void whitelist__err(const char * str, uint32_t n, const char * s, ex_t * e)
{
    if (n <= 99 && strx_is_printablen(str, n))
        ex_set(e, EX_LOOKUP_ERROR, "`%.*s` %s in whitelist", n, str, s);
    else
        ex_set(e, EX_LOOKUP_ERROR, "%s in whitelist", s);
}

static ti_val_t * whitelist__val(ti_val_t * val, ex_t * e)
{
    switch (val->tp)
    {
    case TI_VAL_STR:
    {
        ti_str_t * s = (ti_str_t *) val;
        if (!ti_name_is_valid_strn(s->str, s->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "whitelist is expecting either a regular expression or "
                    "a string according the naming rules"DOC_NAMES);
            return NULL;
        }
        val = (ti_val_t *) ti_names_get(s->str, s->n);
        if (!val)
            ex_set_mem(e);
        return val;
    }
    case TI_VAL_NAME:
    case TI_VAL_REGEX:
        ti_incref(val);
        return val;
    default:
        ex_set(e, EX_TYPE_ERROR,
            "whitelist is expecting "
            "type `"TI_VAL_STR_S"` or `"TI_VAL_REGEX_S"` but "
            "got type `%s` instead",
            ti_val_str(val));
    }
    return NULL;
}

int ti_whitelist_add(vec_t ** whitelist, ti_val_t * val, ex_t * e)
{
    val = whitelist__val(val, e);
    if (!val)
        return e->nr;  /* set by whitelist__val */

    if (*whitelist)
    {
        for (vec_each(*whitelist, ti_val_t, v))
        {
            if (ti_opr_eq(val, v))
            {
                /* this cannot fail for regex and str/name */
                (void) ti_val(val)->to_str(&val, e);
                whitelist__err(
                    ((ti_str_t *) val)->str,
                    ((ti_str_t *) val)->n,
                    "already", e);
                goto fail;
            }
        }
    }
    else
    {
        *whitelist = vec_new(1);
        if (!*whitelist)
            goto failmem;
    }

    if (vec_push(whitelist, val) == 0)
        return 0;

failmem:
    ex_set_mem(e);
fail:
    ti_val_unsafe_drop(val);
    return e->nr;
}

int ti_whitelist_drop(vec_t * whitelist, ti_val_t * val, ex_t * e)
{
    val = whitelist__val(val, e);
    if (!val)
        return e->nr;  /* set by whitelist__val */

    if (whitelist)
    {
        size_t i = 0;
        for (vec_each(whitelist, ti_val_t, v), i++)
        {
            if (ti_opr_eq(val, v))
            {
                (void) vec_swap_remove(whitelist, i);
                ti_val_unsafe_drop(v);
                goto done;
            }
        }
    }

    /* this cannot fail for regex and str/name */
    (void) ti_val(val)->to_str(&val, e);
    whitelist__err(
        ((ti_str_t *) val)->str,
        ((ti_str_t *) val)->n,
        "not found", e);
done:
    ti_val_unsafe_drop(val);
    return e->nr;
}

int ti_whitelist_test(vec_t * whitelist, ti_name_t * name, ex_t * e)
{
    if (!whitelist)
        return 0;   /* allow all if list is NULL */

    for (vec_each(whitelist, ti_val_t, v))
    {
        if (v == (ti_val_t *) name || (ti_val_is_regex(v) &&
                ti_regex_test((ti_regex_t *) v, (ti_raw_t *) name)))
            return 0;
    }

    ex_set(e, EX_FORBIDDEN,
            "no match in whitelist for `%.*s`",
            name->n,
            name->str);
    return e->nr;
}