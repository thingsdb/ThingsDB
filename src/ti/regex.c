/*
 * ti/regex.c
 */
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <util/logger.h>

static ti_regex_t * regex_create(ti_raw_t * re, ex_t * e)
{
    ti_regex_t * regex;
    size_t n = re->n;
    int options = 0;
    int pcre_error_num;
    char * str = (char *) re->data;
    PCRE2_SIZE pcre_error_offset;

    regex = malloc(sizeof(ti_regex_t));
    if (!regex)
    {
        ex_set_mem(e);
        goto fail0;
    }

    regex->ref = 1;
    regex->tp = TI_VAL_REGEX;

    regex->pattern = re;

    while(1)
    {
        switch (str[--n])
        {
        case 'i':
            options |= PCRE2_CASELESS;
            continue;
        case 's':
            options |= PCRE2_DOTALL;
            continue;
        case 'm':
            options |= PCRE2_MULTILINE;
            continue;
        case '/':
            break;
        default:
            ex_set(e, EX_VALUE_ERROR,
                    "unsupported regular expression flag: `%c`",
                    str[n]);
            goto fail0;
        }
        break;
    }

    /* skip the first `/` and shorten the length by one */
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
                regex->pattern->n, (char *) regex->pattern->data);

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
        goto fail1;
    }

    return regex;

fail1:
    pcre2_code_free(regex->code);
fail0:
    free(regex);
    ti_val_unsafe_drop((ti_val_t *) re);
    return NULL;
}

ti_regex_t * ti_regex_create(ti_raw_t * pattern, ti_raw_t * flags, ex_t * e)
{
    size_t sz = sizeof(ti_raw_t) + pattern->n + flags->n + 2;
    ti_raw_t * re = malloc(sz);
    unsigned char * ptr;
    if (!re)
    {
        ex_set_mem(e);
        return NULL;
    }

    ti_raw_init(re, TI_VAL_STR, sz);
    ptr = re->data;

    /* write initial slash */
    *ptr = '/';
    ptr++;

    memcpy(ptr, pattern->data, pattern->n);
    ptr += pattern->n;

    /* write initial slash */
    *ptr = '/';
    ptr++;

    for (size_t i = 0, m = flags->n; i < m; ++i, ++ptr)
    {
        if (!isgraph((*ptr = flags->data[i])))
        {
            ti_val_unsafe_drop((ti_val_t *) re);
            ex_set(e, EX_VALUE_ERROR, "invalid regular expression flags");
            return NULL;
        }
    }
    return regex_create(re, e);
}

ti_regex_t * ti_regex_from_strn(const char * str, size_t n, ex_t * e)
{
    ti_raw_t * re = ti_str_create(str, n);
    if (!re)
    {
        ex_set_mem(e);
        return NULL;
    }
    return regex_create(re, e);
}

void ti_regex_destroy(ti_regex_t * regex)
{
    if (!regex)
        return;

    pcre2_match_data_free(regex->match_data);
    pcre2_code_free(regex->code);
    ti_val_drop((ti_val_t *) regex->pattern);
    free(regex);
}
