/*
 * raw.c
 */
#include <assert.h>
#include <string.h>
#include <ti/raw.h>

ti_raw_t * ti_raw_new(const unsigned char * raw, size_t n)
{
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + n);
    if (!r)
        return NULL;
    r->n = n;
    memcpy(r->data, raw, n);
    return r;
}

ti_raw_t * ti_raw_dup(const ti_raw_t * raw)
{
    size_t sz = sizeof(ti_raw_t) + raw->n;
    ti_raw_t * r = malloc(sz);
    if (!r)
        return NULL;
    memcpy(r, raw, sz);
    return r;
}

ti_raw_t * ti_raw_from_packer(qp_packer_t * packer)
{
    size_t sz = sizeof(ti_raw_t) + packer->len;
    ti_raw_t * r = malloc(sz);
    if (!r)
        return NULL;
    r->n = packer->len;
    memcpy(r->data, packer->buffer, packer->len);
    return r;
}

ti_raw_t * ti_raw_from_ti_string(const char * src, size_t n)
{
    assert (n >= 2);  /* at least "" or '' */

    size_t i = 0;
    size_t sz = n - 2;
    ti_raw_t * r = malloc(sizeof(ti_raw_t) + sz);
    if (!r)
        return NULL;

    /* take the first character, this is " or ' */
    char chr = *src;

    /* we need to loop till len-2 so take 2 */
    for (n -= 2; i < n; i++)
    {
        src++;
        if (*src == chr)
        {
            /* this is the character, skip one and take the next */
            src++;
            n--;
        }
        r->data[i] = *src;
    }

    r->n = i;

    if (r->n < sz)
    {
        ti_raw_t * tmp = realloc(r, sizeof(ti_raw_t) + r->n);
        if (tmp)
            r = tmp;
    }

    return r;
}

char * ti_raw_to_str(const ti_raw_t * raw)
{
    char * str = malloc(raw->n + 1);
    if (!str)
        return NULL;
    memcpy(str, raw->data, raw->n);
    str[raw->n] = '\0';
    return str;
}




