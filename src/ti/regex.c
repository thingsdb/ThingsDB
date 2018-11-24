/*
 * ti/regex.c
 */
#include <assert.h>
#include <util/logger.h>
#include <ti/regex.h>
#include <stdlib.h>


ti_regex_t * ti_regex_from_strn(const char * str, size_t n, ex_t * e)
{
    ti_regex_t * regex;
    int options = 0;
    int pcre_error_num;
    PCRE2_SIZE pcre_error_offset;

    regex = malloc(sizeof(ti_regex_t));
    if (!regex)
    {
        ex_set_alloc(e);
        goto fail0;
    }

    regex->ref = 1;
    regex->pattern = ti_raw_create((uchar *) str, n);
    if (!regex->pattern)
    {
        ex_set_alloc(e);
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
        ex_set(e, EX_BAD_DATA, "cannot compile regular expression '%.*s': ",
                (int) regex->pattern->n,
                (char *) regex->pattern->data);

        pcre2_get_error_message(
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
        ex_set_alloc(e);
        goto fail2;
    }

    return regex;

fail2:
    pcre2_code_free(regex->code);
fail1:
    ti_raw_drop(regex->pattern);
fail0:
    free(regex);
    return NULL;
}

void ti_regex_drop(ti_regex_t * regex)
{
    if (regex && !--regex->ref)
    {
        pcre2_match_data_free(regex->match_data);
        pcre2_code_free(regex->code);
        ti_raw_drop(regex->pattern);
        free(regex);
    }
}

