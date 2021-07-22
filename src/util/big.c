/*
 * util/big.c
 */
#include <assert.h>
#include <util/big.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <math.h>

#define BIG__MMIN 0x8000000000000000ULL
#define BIG__U32MASK 0xffffffffUL
#define BIG__LEFT(__u64) ((__u64 & 0xffffffff00000000ULL) >> 32)
#define BIG__RIGHT(__u64) (__u64 & BIG__U32MASK)


int big_to_str16n(const big_t * big, char * tostr, size_t size)
{
    ssize_t sz = (ssize_t) size;
    int count;
    uint16_t n = 0;

    if (!big->n_)
        return snprintf(tostr, size, "0");

    assert (big->parts_[n]);

    count = snprintf(tostr, size, "%"PRIx32, big->parts_[n]);

    if (count < 0)
        return count;

    sz = count >= sz ? 0 : sz - count;

    for (++n; n < big->n_ ; ++n)
    {
        int rc = snprintf(tostr + count, sz, "%08"PRIx32, big->parts_[n]);

        if (rc > 0)
            count += rc;
        else
            return rc;

        sz = rc >= sz ? 0 : sz - rc;
    }

    return count;
}

big_t * big_from_str2n(const char * str, size_t n)
{
    big_t * big;
    uint32_t m, * ptr;

    /* skip leading zero's */
    while (*str == '0')
    {
        ++str;
        --n;
    }

    m = (uint32_t) ceil(n / 32.0);
    big = calloc(1, sizeof(big_t) + sizeof(uint32_t) * m);
    if (!big)
        return NULL;

    big->n_ = m;
    big->negative_ = 0;

    ptr = big->parts_;
    while (n--)
    {
        m = n % 32;
        if (*str == '1')
        {
            *ptr |= 1 << m;
        }
        else if (*str != '0')
        {
            errno = EINVAL;
            break;
        }

        if (!m)
            ++ptr;

        ++str;
    }

    return big;
}

big_t * big_from_str8n(const char * str, size_t n)
{
    big_t * big;
    uint32_t offset, m, * ptr;
    int c;

    /* skip leading zero's */
    while (*str == '0')
    {
        ++str;
        --n;
    }

    m = (uint32_t) ceil(n / 10.665f);
    big = calloc(1, sizeof(big_t) + sizeof(uint32_t) * m);
    if (!big)
        return NULL;

    big->n_ = m;
    big->negative_ = 0;
    offset = n * 3 - 1;

    ptr = big->parts_;
    while (n--)
    {
        m = offset % 32;
        c = *str - '0';
        if (m == 0)
        {

        }
        if (m == 1)
        {
            *ptr |= c;
        }
        else if (m == 2)
        {

        }
        else if (m == 3)
        {

        }
        else
        {
            *ptr |= c << m;
        }
        offset -= 3;
        if (!m)
            ++ptr;

        ++str;
    }

    return big;
}

big_t * big_null(void)
{
    big_t * big = malloc(sizeof(big_t));
    if (!big)
        return NULL;
    big->n_ = 0;
    big->negative_ = 0;
    return big;
}

big_t * big_from_int64(const int64_t i)
{
    big_t * big;
    uint32_t count, lu;
    uint64_t u = i == LLONG_MIN ? BIG__MMIN : (uint64_t) llabs(i);

    lu = BIG__LEFT(u);
    count = lu ? 2 : i ? 1 : 0;
    big = malloc(sizeof(big_t) + sizeof(uint32_t) * count);
    if (!big)
        return NULL;

    big->negative_ = i < 0;
    big->n_ = count;

    if (lu)
    {
        big->parts_[0] = lu;
        big->parts_[1] = BIG__RIGHT(u);
    }
    else if (i)
    {
        big->parts_[0] = BIG__RIGHT(u);
    }

    return big;
}

int64_t big_to_int64(const big_t * big)
{
    assert (big_fits_int64(big));
    int64_t i;
    uint64_t u = big->parts_[0];

    u <<= 32;
    u += big->parts_[1];
    i = (int64_t) u;

    return big->negative_ ? -i : i;
}

big_t * big_mulii(const int64_t a, const int64_t b)
{
    big_t * big;
    uint32_t start, count, size, parts[4];
    uint64_t a0, a1, b0, b1, part;

    if (!a || !b)
        return big_null();

    a0 = a == LLONG_MIN ? BIG__MMIN : (uint64_t) llabs(a);
    b0 = b == LLONG_MIN ? BIG__MMIN : (uint64_t) llabs(b);

    a1 = BIG__LEFT(a0);
    b1 = BIG__LEFT(b0);;

    a0 &= BIG__U32MASK;
    b0 &= BIG__U32MASK;

    part = a0*b0;

    parts[3] = BIG__RIGHT(part);
    parts[2] = BIG__LEFT(part);

    part = a1*b0 + parts[2];;
    parts[2] = BIG__RIGHT(part);
    parts[1] = BIG__LEFT(part);

    part = a0*b1 + parts[2];
    parts[2] = BIG__RIGHT(part);
    parts[1] += BIG__LEFT(part);

    part = a1*b1 + parts[1];
    parts[1] = BIG__RIGHT(part);
    parts[0] = BIG__LEFT(part);

    for (start = 0; start < 4 && !parts[start]; ++start);
    count = (4 - start);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = (a<0) ^ (b<0);
    big->n_ = count;
    memcpy(big->parts_, parts + start, size);

    return big;
}

big_t * big_mulbb(const big_t * a, const big_t * b)
{
    big_t * big;
    uint32_t nmax = a->n_ + b->n_;
    uint32_t i = nmax - 1, count, size, parts[nmax];
    uint64_t part;

    if (big_is_null(a) || big_is_null(b))
        return big_null();

    for (count = 0; count < nmax; ++count)
        parts[count] = 0;

    for (uint_fast16_t an = a->n_; an--; --i)
    {
        for (uint_fast16_t bn = b->n_, x = i; bn--; )
        {
            part = (uint64_t) a->parts_[an]*(uint64_t) b->parts_[bn]+parts[x];
            parts[x] = BIG__RIGHT(part);
            parts[--x] += BIG__LEFT(part);
        }
    }

    for (i = 0; i < nmax && !parts[i]; ++i);
    count = (nmax - i);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = a->negative_ ^ b->negative_;
    big->n_ = count;
    memcpy(big->parts_, parts + i, size);

    return big;
}

big_t * big_mulbi(const big_t * a, const int64_t b)
{
    big_t * big;
    uint32_t nmax = a->n_ + 2;
    uint32_t i = nmax - 1, count, size, parts[nmax];
    uint64_t b0, b1, part;

    if (big_is_null(a) || !b)
        return big_null();

    b0 = b == LLONG_MIN ? BIG__MMIN : (uint64_t) llabs(b);
    b1 = BIG__LEFT(b0);
    b0 &= BIG__U32MASK;

    for (unsigned int k = 0; k < nmax; ++k)
        parts[k] = 0;

    for (uint_fast16_t an = a->n_; an--; ++i)
    {
        part = (uint64_t) a->parts_[an]*b0 + parts[i];
        parts[i] = BIG__RIGHT(part);
        parts[--i] += BIG__LEFT(part);

        part = (uint64_t) a->parts_[an]*b1 + parts[i];
        parts[i] = BIG__RIGHT(part);
        parts[--i] += BIG__LEFT(part);
    }

    for (i = 0; i < nmax && !parts[i]; ++i);
    count = (nmax - i);
    size = count * sizeof(uint32_t);

    big = malloc(sizeof(big_t) + size);
    if (!big)
        return NULL;

    big->negative_ = a->negative_^(b<0);
    big->n_ = count;
    memcpy(big->parts_, parts + i, size);

    return big;
}

double big_mulbd(const big_t * a, const double b)
{
    double r = 0.0, d;
    uint_fast16_t i, n, k;
    for (i = 0, n = a->n_; i < n; ++i)
    {
        d = b * (double) a->parts_[i];
        k = n - i;
        while (--k)
            d *= (double) 0x100000000ULL;
        r += d;
    }
    return a->negative_ ? -r : r;
}


/*
static void addDecValue (int * pHexArray, int nElements, int value)
{
    int carryover = value;
    int tmp = 0;
    int i;

    for (i = (nElements-1); (i >= 0); i--)
    {
        tmp = (pHexArray[i] * 10) + carryover;
        pHexArray[i] = tmp % 16;
        carryover = tmp / 16;
    }
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys/types.h"

char HexChar [16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static int * initHexArray (char * pDecStr, int * pnElements);

static void addDecValue (int * pMyArray, int nElements, int value);
static void printHexArray (int * pHexArray, int nElements);

static int * initHexArray (char * pDecStr, int * pnElements)
{
    int * pArray = NULL;
    int lenDecStr = strlen (pDecStr);
    int i;

    pArray = (int *) calloc (lenDecStr,  sizeof (int));

    for (i = 0; i < lenDecStr; i++)
    {
        addDecValue (pArray, lenDecStr, pDecStr[i] - '0');
    }

    *pnElements = lenDecStr;

    return (pArray);
}

static void printHexArray (int * pHexArray, int nElements)
{
    int start = 0;
    int i;

    while ((pHexArray[start] == 0) && (start < (nElements-1)))
    {
        start++;
    }

    for (i = start; i < nElements; i++)
    {
        printf ("%c", HexChar[pHexArray[i]]);
    }

    printf ("\n");
}

*/
