/*
 * ti/regex.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <util/logger.h>


ti_regex_t * ti_regex_from_strn(const char * str, size_t n, ex_t * e)
{
    ti_regex_t * regex;
    int options = 0;
    int pcre_error_num;
    PCRE2_SIZE pcre_error_offset;

    regex = malloc(sizeof(ti_regex_t));
    if (!regex)
    {
        ex_set_mem(e);
        goto fail0;
    }

    regex->ref = 1;
    regex->tp = TI_VAL_REGEX;

    regex->pattern = ti_str_create(str, n);
    if (!regex->pattern)
    {
        ex_set_mem(e);
        goto fail0;
    }

    --n;
    if (str[n] == 'i')
    {
        options |= PCRE2_CASELESS;
        --n;
    }

    /* check first and last slashes */
    assert (str[0] == '/');
    assert (str[n] == '/');

    ++str;
    --n;

    regex->code = pcre2_compile(
        (PCRE2_SPTR8) str,
        n,
        options,
        &pcre_error_num,
        &pcre_error_offset,
        NULL);

    if (!regex->code)
    {
        ex_set(e, EX_VALUE_ERROR, "cannot compile regular expression '%.*s', ",
                (int) regex->pattern->n,
                (char *) regex->pattern->data);

        e->n += pcre2_get_error_message(
                pcre_error_num,
                (PCRE2_UCHAR *) e->msg + e->n,
                EX_MAX_SZ - e->n);

        goto fail1;
    }

    regex->match_data = pcre2_match_data_create_from_pattern(
            regex->code,
            NULL);

    if (!regex->match_data)
    {
        ex_set_mem(e);
        goto fail2;
    }

    return regex;

fail2:
    pcre2_code_free(regex->code);
fail1:
    ti_val_unsafe_drop((ti_val_t *) regex->pattern);
fail0:
    free(regex);
    return NULL;
}

void ti_regex_destroy(ti_regex_t * regex)
{
    if (!regex)
        return;

    pcre2_match_data_free(regex->match_data);
    pcre2_code_free(regex->code);
    ti_val_safe_drop((ti_val_t *) regex->pattern);
    free(regex);
}
