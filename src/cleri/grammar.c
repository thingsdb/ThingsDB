/*
 * grammar.c - this should contain the 'start' or your grammar.
 */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <cleri/grammar.h>
#include <stdlib.h>
#include <stdio.h>
#include <pcre2.h>
#include <assert.h>

/*
 * Returns a grammar object or NULL in case of an error.
 *
 * Warning: this function could write to stderr in case the re_keywords could
 * not be compiled.
 */
cleri_grammar_t * cleri_grammar2(
        cleri_t * start,
        const char * re_keywords,
        const char * re_whitespace)
{
    const char * re_kw = (re_keywords == NULL) ?
            CLERI_DEFAULT_RE_KEYWORDS : re_keywords;
    const char * re_ws = (re_whitespace == NULL) ?
            CLERI_DEFAULT_RE_WHITESPACE : re_whitespace;

    /* re_keywords should start with a ^ */
    assert (re_kw[0] == '^');

    if (start == NULL)
    {
        return NULL;
    }

    cleri_grammar_t * grammar = cleri__malloc(cleri_grammar_t);
    if (grammar == NULL)
    {
        return NULL;
    }

    int pcre_error_num;
    PCRE2_SIZE pcre_error_offset;

    grammar->re_keywords = pcre2_compile(
            (PCRE2_SPTR8) re_kw,
            PCRE2_ZERO_TERMINATED,
            0,
            &pcre_error_num,
            &pcre_error_offset,
            NULL);
    if(grammar->re_keywords == NULL)
    {

        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(pcre_error_num, buffer, sizeof(buffer));
        /* this is critical and unexpected, memory is not cleaned */
        fprintf(stderr,
                "failed to compile keywords '%s' (%s)\n",
                re_kw,
                buffer);
        goto fail0;
    }

    grammar->md_keywords = \
        pcre2_match_data_create_from_pattern(grammar->re_keywords, NULL);

    if (grammar->md_keywords == NULL)
    {
        fprintf(stderr, "failed to create keyword match data\n");
        goto fail1;
    }

    grammar->re_whitespace = pcre2_compile(
            (PCRE2_SPTR8) re_ws,
            PCRE2_ZERO_TERMINATED,
            0,
            &pcre_error_num,
            &pcre_error_offset,
            NULL);
    if(grammar->re_whitespace == NULL)
    {

        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(pcre_error_num, buffer, sizeof(buffer));
        /* this is critical and unexpected, memory is not cleaned */
        fprintf(stderr,
                "failed to compile whitespace '%s' (%s)\n",
                re_ws,
                buffer);
        goto fail1;
    }

    grammar->md_whitespace = \
        pcre2_match_data_create_from_pattern(grammar->re_whitespace, NULL);

    if (grammar->md_whitespace == NULL)
    {
        fprintf(stderr, "failed to create whitespace match data\n");
        goto fail2;
    }

    /* bind root element and increment the reference counter */
    grammar->start = start;
    cleri_incref(start);

    return grammar;

fail2:
    pcre2_code_free(grammar->re_whitespace);
fail1:
    pcre2_code_free(grammar->re_keywords);
fail0:
    free(grammar);
    return NULL;
}

cleri_grammar_t * cleri_grammar(cleri_t * start, const char * re_keywords)
{
    return cleri_grammar2(start, re_keywords, NULL);
}

void cleri_grammar_free(cleri_grammar_t * grammar)
{
    pcre2_match_data_free(grammar->md_keywords);
    pcre2_code_free(grammar->re_keywords);
    pcre2_match_data_free(grammar->md_whitespace);
    pcre2_code_free(grammar->re_whitespace);
    cleri_free(grammar->start);
    free(grammar);
}
