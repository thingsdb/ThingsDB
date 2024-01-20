/*
 * kwcache.c - holds keyword regular expression result while parsing.
 */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <stdlib.h>
#include <pcre2.h>
#include <string.h>
#include <cleri/kwcache.h>
#include <assert.h>

#define NOT_FOUND UINT8_MAX


/*
 * Returns NULL in case an error has occurred.
 */
uint8_t * cleri__kwcache_new(const char * str)
{
    size_t n = strlen(str);
    uint8_t * kwcache = cleri__mallocn(n, uint8_t);
    if (kwcache != NULL)
    {
        memset(kwcache, NOT_FOUND, n * sizeof(uint8_t));
    }
    return kwcache;
}

/*
 * Returns 0 when no kw_match is found, -1 when an error has occurred, or the
 * new kwcache->len value.
 */
uint8_t cleri__kwcache_match(cleri_parse_t * pr, const char * str)
{
    uint8_t * len;

    if (*str == '\0')
    {
        return 0;
    }

    len = &pr->kwcache[str - pr->str];

    if (*len == NOT_FOUND)
    {
        int pcre_exec_ret = pcre2_match(
                    pr->grammar->re_keywords,
                    (PCRE2_SPTR8) str,
                    PCRE2_ZERO_TERMINATED,
                    0,                     // start looking at this point
                    0,                     // OPTIONS
                    pr->grammar->md_keywords,
                    NULL);

        *len = pcre_exec_ret < 0
            ? 0
            : pcre2_get_ovector_pointer(pr->grammar->md_keywords)[1];
    }
    return *len;
}

/*
 * Destroy kwcache. (parsing NULL is allowed)
 */
void cleri__kwcache_free(uint8_t * kwcache)
{
    free(kwcache);
}
