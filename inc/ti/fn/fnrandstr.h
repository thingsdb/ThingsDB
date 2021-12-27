#include <ti/fn/fn.h>

#define TI_RANDSTR_MAX 1024

static int do__f_randstr(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    int64_t n;
    ti_raw_t * charset;
    unsigned char * buffer;

    if (fn_nargs_range("randstr", DOC_RANDSTR, 1, 2, nargs, e) ||
        ti_do_statement(query, nd->children, e))
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
        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_str("randstr", DOC_RANDSTR, 2, query->rval, e))
            return e->nr;

        charset = (ti_raw_t *) query->rval;

        if (!charset->n)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "function `randstr` requires a character set with "
                    "at least one valid ASCII character"DOC_RANDSTR);
            return e->nr;
        }

        if (!strx_is_asciin((const char *) charset->data, charset->n))
        {
            ex_set(e, EX_VALUE_ERROR,
                    "function `randstr` requires a character set with ASCII "
                    "characters only"DOC_RANDSTR);
            return e->nr;
        }

        query->rval = NULL;
    }
    else
    {
        charset = (ti_raw_t *) ti_val_charset_str();
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
    ti_raw_init((ti_raw_t *) buffer, TI_VAL_STR, n);
    query->rval = (ti_val_t *) buffer;

    buffer += sizeof(ti_raw_t);
    n -= sizeof(ti_raw_t);

    if (256 % charset->n == 0)
    {
        /*
         * Depending on the length of the character length we can use this
         * optimal code. This only works well if 265 module the length is 0
         * since only then each character in the set has the same change to
         * appear in result.
         */
        util_get_random(buffer, n);
        while (n--)
            buffer[n] = charset->data[buffer[n] % charset->n];
    }
    else
    {
        /*
         * Get random data in batches and fill the buffer based on the
         * data.
         */
        const size_t tmp_sz = 64;
        unsigned int tmprand[tmp_sz];

        while (n)
        {
            util_get_random(tmprand, sizeof(tmprand));
            for(size_t i = 0; n && i < tmp_sz; ++i)
                buffer[--n] = charset->data[tmprand[i] % charset->n];
        }
    }

fail0:
    ti_val_unsafe_drop((ti_val_t *) charset);
    return e->nr;
}
