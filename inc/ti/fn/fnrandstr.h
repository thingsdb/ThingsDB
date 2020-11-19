#include <ti/fn/fn.h>

#define TI_RANDSTR_MAX 1024

static int do__f_randstr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    int64_t n;
    ti_raw_t * charset;
    void * buffer, * end;

    if (fn_nargs_range("randstr", DOC_RANDSTR, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children->node, e))
        return e->nr;

    if (fn_arg_int("randstr", DOC_RANDSTR, 1, query->rval, e))
        return e->nr;

    n = VINT(query->rval);

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (n < 0 || n > TI_RANDSTR_MAX)
    {
        ex_set(e, EX_VALUE_ERROR,
            "function `randstr` requires a length between 0 and %d"DOC_RANDSTR,
            TI_RANDSTR_MAX);
        return e->nr;
    }

    if (nargs == 2)
    {
        if (ti_do_statement(query, nd->children->next->next->node, e) ||
            fn_arg_str("randstr", DOC_RANDSTR, 2, query->rval, e))
            return e->nr;

        charset = (ti_raw_t *) query->rval;

        if (!charset->n)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "function `randstr` requires a character set to contain "
                    "at least one valid ASCII character"DOC_RANDSTR);
            return e->nr;
        }

        if (!strx_is_asciin((const char *) charset->data, charset->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "function `randstr` requires a character set with ASCII "
                    "characters only"
                    DOC_RANDSTR);
            return e->nr;
        }

        query->rval = NULL;
    }
    else
    {
        charset = ti_val_charset_str();
    }

    n += sizeof(ti_raw_t);

    buffer = malloc(n);
    if (!buffer)
    {
        ex_set_mem(e);
        goto fail0;
    }

    /*
     * Set the return value to the buffer.
     * This allows the code to change the buffer and actually fill the buffer
     * with the correct data.
     */
    ti_raw_init(buffer, TI_VAL_STR, n);
    query->rval = buffer;

    end = buffer + n;
    buffer += sizeof(ti_raw_t);

    while (buffer < end)
    {
        *buffer = charset->data[rand() % charset->n];
        ++buffer;
    }

fail0:
    ti_val_drop_undafe((ti_val_t *) charset);
    return e->nr;
}
