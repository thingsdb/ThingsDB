/*
 * vec.c
 */
#include <assert.h>
#include <string.h>
#include <util/vec.h>

/*
 * Returns a new vector with a given size.
 */
vec_t * vec_new(uint32_t sz)
{
    vec_t * vec = malloc(sizeof(vec_t) + sz * sizeof(void*));
    if (!vec)
        return NULL;
    vec->sz = sz;
    vec->n = 0;
    return vec;
}

/*
 * Destroy a vector with optional callback. If callback is NULL then it is
 * just as safe to simply call free() instead of this function.
 */
void vec_destroy(vec_t * vec, vec_destroy_cb cb)
{
    if (vec && cb)
        for (vec_each(vec, void, obj), (*cb)(obj));
    free(vec);
}
#include <util/logger.h>

static inline uint32_t vec__gcd(uint32_t a, uint32_t b)
{
    /* do-while may be used since both a != 0 and b != 0 */
    do
    {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    while (b);
    return a;
}

void vec_move(vec_t * vec, uint32_t pos, uint32_t n, uint32_t to)
{
    uint32_t range, gcd, m, i, j, _;
    void * tmp;

    if (to == pos || !n)
        return;  /* no move required */

    if (to < pos)
    {
        /* swap, this could be done with a recursive call as well */
        _ = to + n;
        n = pos - to;
        pos = to;
        to = _;
    }

    assert (to + n <= vec->n);

    /* calculate the range involved in the move */
    range = to + n - pos;

    /* calculate the greatest common divisor */
    gcd = vec__gcd(range, n);

    /* inner loop cycle based on the range and greatest common divisor */
    m = range / gcd - 1;

    while (gcd--)
    {
        i = gcd;
        tmp = vec->data[i + pos];
        for (_ = 0; _ < m; ++_)
        {
            j = (i + n) % range;
            vec->data[i + pos] = vec->data[j + pos];
            i = j;
        }
        vec->data[i + pos] = tmp;
    }
}

void vec_reverse(vec_t * vec)
{
    size_t i, n, e;

    for (i = 0, n = vec->n, e = n / 2; i < e; ++i)
    {
        void * tmp = vec->data[i];
        vec->data[i] = vec->data[--n];
        vec->data[n] = tmp;
    }
}

void * vec_remove(vec_t * vec, uint32_t i)
{
    void * data = VEC_get(vec, i);
    memmove(vec->data + i, vec->data + i + 1, (--vec->n - i) * sizeof(void*));
    return data;
}

void * vec_swap_remove(vec_t * vec, uint32_t i)
{
    assert (vec->n > 0);
    void * data = VEC_get(vec, i);
    vec->data[i] = vec->data[--vec->n];
    return data;
}

/*
 * Returns a copy of a given vector with an exact fit so the new size and `n`
 * will be equal. In case of an allocation error the return value is NULL.
 */
vec_t * vec_dup(const vec_t * vec)
{
    size_t sz = sizeof(vec_t) + vec->n * sizeof(void*);
    vec_t * v = malloc(sz);
    if (!v)
        return NULL;

    memcpy(v, vec, sz);
    v->sz = v->n;
    return v;
}

/*
 * Returns a copy of a given vector with equal size.
 * In case of an allocation error the return value is NULL.
 */
vec_t * vec_copy(const vec_t * vec)
{
    size_t sz = sizeof(vec_t) + vec->sz * sizeof(void*);
    vec_t * v = malloc(sz);
    if (!v)
        return NULL;

    memcpy(v, vec, sz);
    return v;
}

/*
 * Returns 0 when successful.
 */
int vec_push(vec_t ** vaddr, void * data)
{
    vec_t * vec = *vaddr;
    if (vec->n == vec->sz)
    {
        size_t prev = vec->sz;

        if (prev)
            vec->sz <<= 1;
        else
            vec->sz = 1;  /* the first item */

        vec_t * tmp = realloc(vec, sizeof(vec_t) + vec->sz * sizeof(void*));

        if (!tmp)
        {
            /* restore original size */
            vec->sz = prev;
            return -1;
        }

        *vaddr = vec = tmp;
    }
    VEC_push(vec, data);
    return 0;
}

/*
 * Returns 0 when successful.
 */
int vec_insert(vec_t ** vaddr, void * data, uint32_t i)
{
    vec_t * vec = *vaddr;
    assert(i <= vec->n);
    if (vec->n == vec->sz)
    {
        size_t prev = vec->sz;

        if (prev)
            vec->sz <<= 1;
        else
            vec->sz = 1;  /* the first item */

        vec_t * tmp = realloc(vec, sizeof(vec_t) + vec->sz * sizeof(void*));

        if (!tmp)
        {
            /* restore original size */
            vec->sz = prev;
            return -1;
        }

        *vaddr = vec = tmp;
    }

    if (i < vec->n)
        memmove(vec->data+i+1, vec->data+i, (vec->n-i) * sizeof(void*));

    VEC_set(vec, data, i);
    ++vec->n;

    return 0;
}

/*
 * Returns 0 when successful.
 */
int vec_extend(vec_t ** vaddr, void * data[], uint32_t n)
{
    vec_t * vec = *vaddr;
    vec->n += n;
    if (vec->n > vec->sz)
    {
        vec_t * tmp = realloc(vec, sizeof(vec_t) + vec->n * sizeof(void*));
        if (!tmp)
        {
            /* restore original length */
            vec->n -= n;
            return -1;
        }

        *vaddr = vec = tmp;
        vec->sz = vec->n;
    }
    memcpy(vec->data + (vec->n - n), data, n * sizeof(void*));
    return 0;
}

/*
 * Make at least `n` free spots in the vector.
 *
 * If *vaddr is NULL, a new vector will be created with the required space.
 *
 * More spots might be allocated and nothing happens if enough empty
 * spots are already available.
 */
int vec_make(vec_t ** vaddr, uint32_t n)
{
    if (!*vaddr)
    {
        *vaddr = vec_new(n);
        return -!*vaddr;
    }
    return vec_reserve(vaddr, n);
}

/*
 * Reserve at least `n` free spots in the vector.
 *
 * More spots might be allocated and nothing happens if enough empty
 * spots are already available.
 */
int vec_reserve(vec_t ** vaddr, uint32_t n)
{
    vec_t * vec = *vaddr;

    if (vec_space(vec) < n)
    {
        vec_t * tmp;
        size_t sz = vec->sz << 1;

        vec->sz = (sz - vec->n) < n ? n : sz;

        tmp = realloc(vec, sizeof(vec_t) + vec->sz * sizeof(void*));
        if (!tmp)
        {
            /* restore original size */
            vec->sz = sz >> 1;
            return -1;
        }

        *vaddr = vec = tmp;
    }
    return 0;
}

/*
 * Resize a vector to a given size.
 *
 * If the new size is smaller then `n` might decrease.
 */
int vec_resize(vec_t ** vaddr, uint32_t sz)
{
    vec_t * vec = *vaddr;
    vec_t * v = realloc(vec, sizeof(vec_t) + sz * sizeof(void*));
    if (!v)
        return -1;

    if (v->n > sz)
        v->n = sz;

    v->sz = sz;
    *vaddr = v;
    return 0;
}

/*
 * May shrink to some size if a lot is reserved.
 *
 * Shrinks to a size, plus 8 extra, if 16 or more spots are free.
 *
 * Returns 0 when successful.
 */
int vec_may_shrink(vec_t ** vaddr)
{
    vec_t * vec = *vaddr;
    if (vec_space(vec) < 16)
        return 0;

    vec_t * v = realloc(vec, sizeof(vec_t) + (vec->n+8) * sizeof(void*));
    if (!v)
        return -1;

    v->sz = v->n;
    *vaddr = v;
    return 0;
}

/*
 * Shrinks a vector to an exact fit.
 *
 * Returns 0 when successful.
 */
int vec_shrink(vec_t ** vaddr)
{
    vec_t * vec = *vaddr;
    if (vec->n == vec->sz)
        return 0;

    vec_t * v = realloc(vec, sizeof(vec_t) + vec->n * sizeof(void*));
    if (!v)
        return -1;

    v->sz = v->n;
    *vaddr = v;
    return 0;
}

static void * vec__sort_arg;
static vec_sort_r_cb vec__sort_cb;
static inline int vec__compare_cb(const void ** a, const void ** b)
{
    return vec__sort_cb(*a, *b, vec__sort_arg);
}

/* careful: this function is NOT thread safe */
void vec_sort_r(vec_t * vec, vec_sort_r_cb compare, void * arg)
{
    vec__sort_arg = arg;
    vec__sort_cb = compare;
    vec_sort(vec, vec__compare_cb);
    vec__sort_cb = NULL;
}

_Bool vec_is_sorting(void)
{
    return !!vec__sort_cb;
}
