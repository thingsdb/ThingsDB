/*
 * util/vec.h
 */
#ifndef VEC_H_
#define VEC_H_

typedef struct vec_s  vec_t;
typedef void (*vec_destroy_cb)(void *);
typedef int (*vec_sort_cb) (const void **, const void **);
typedef int (*vec_sort_r_cb) (const void *, const void *, void *);

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

vec_t * vec_new(uint32_t sz);
void vec_destroy(vec_t * vec, vec_destroy_cb cb);
static inline uint32_t vec_space(const vec_t * vec);
static inline void * vec_first(const vec_t * vec);
static inline void * vec_last(const vec_t * vec);
static inline void * vec_get(const vec_t * vec, uint32_t i);
static inline void * vec_set(vec_t * vec, void * data, uint32_t i);
static inline void ** vec_get_addr(vec_t * vec, uint32_t i);
static inline void * vec_pop(vec_t * vec);
static inline void vec_clear(vec_t * vec);
static inline void vec_clear_cb(vec_t * vec, vec_destroy_cb cb);
static inline void vec_swap(vec_t * vec, uint32_t i, uint32_t j);
void vec_move(vec_t * vec, uint32_t pos, uint32_t n, uint32_t to);
void vec_reverse(vec_t * vec);
void * vec_remove(vec_t * vec, uint32_t i);
void * vec_swap_remove(vec_t * vec, uint32_t i);
vec_t * vec_dup(const vec_t * vec);
vec_t * vec_copy(const vec_t * vec);
int vec_push(vec_t ** vaddr, void * data);
int vec_insert(vec_t ** vaddr, void * data, uint32_t i);
int vec_extend(vec_t ** vaddr, void * data[], uint32_t n);
int vec_make(vec_t ** vaddr, uint32_t n);
int vec_reserve(vec_t ** vaddr, uint32_t n);
int vec_resize(vec_t ** vaddr, uint32_t sz);
int vec_shrink(vec_t ** vaddr);
int vec_may_shrink(vec_t ** vaddr);
static inline void vec_sort(vec_t * vec, vec_sort_cb compare);
void vec_sort_r(vec_t * vec, vec_sort_r_cb compare, void * arg);
_Bool vec_is_sorting(void);

static inline void * VEC_get(const vec_t * vec, uint32_t i);

#define VEC_set(vec__, data__, i__) ((vec__)->data[i__] = data__)

/* unsafe macro for vec_push() which assumes the vector has enough space */
#define VEC_push(vec__, data_) ((vec__)->data[(vec__)->n++] = data_)

/* unsafe macro for vec_push() which assumes the vector has enough space */
#define VEC_pop(vec__) (vec__)->data[--(vec__)->n]

#define VEC_first(vec__) (vec__)->data[0]

#define VEC_last(vec__) (vec__)->data[(vec__)->n-1]

/* use vec_each in a for loop to go through all values by it's address */
#define vec_each_addr(vec__, dt__, var__) \
    dt__ ** var__ = (dt__ **) (vec__)->data, \
         ** end__ = var__ + (vec__)->n; \
    var__ < end__; \
    var__++

/* use vec_each in a for loop to go through all values.
 * do not change the vector while iterating over the values */
#define vec_each(vec__, dt__, var__) \
    dt__ * var__, \
    ** v__ = (dt__ **) (vec__)->data, \
    ** e__ = v__ + (vec__)->n; \
    v__ < e__ && ((var__ = *v__) || 1); \
    v__++

/* use vec_each_rev in a for loop to go through all values reversed.
 * do not change the vector while iterating over the values */
#define vec_each_rev(vec__, dt__, var__) \
    dt__ * var__, \
    ** e__ = (dt__ **) (vec__)->data, \
    ** v__ = e__ + (vec__)->n; \
    v__ > e__ && ((var__ = *(--v__)) || 1);

struct vec_s
{
    uint32_t n;
    uint32_t sz;
    void * data[];
};

static inline uint32_t vec_space(const vec_t * vec)
{
    return vec->sz - vec->n;
}

static inline void * VEC_get(const vec_t * vec, uint32_t i)
{
    return vec->data[i];
}

static inline void * vec_get(const vec_t * vec, uint32_t i)
{
    return i < vec->n ? vec->data[i] : NULL;
}

static inline void * vec_first(const vec_t * vec)
{
    return vec_get(vec, 0);
}

static inline void * vec_last(const vec_t * vec)
{
    return vec_get(vec, vec->n - 1);
}

static inline void * vec_set(vec_t * vec, void * data, uint32_t i)
{
    void * old = vec->data[i];
    vec->data[i] = data;
    return old;
}

static inline void ** vec_get_addr(vec_t * vec, uint32_t i)
{
    return vec->data + i;
}

static inline void * vec_pop(vec_t * vec)
{
    return vec->n ? vec->data[--vec->n] : NULL;
}

static inline void vec_clear(vec_t * vec)
{
    vec->n = 0;
}

static inline void vec_clear_cb(vec_t * vec, vec_destroy_cb cb)
{
    for (vec_each(vec, void, v))
        cb(v);
    vec->n = 0;
}

typedef int (*__vec_sort_cb) (const void *, const void *);

static inline void vec_sort(vec_t * vec, vec_sort_cb compare)
{
    qsort(vec->data, vec->n, sizeof(void *), (__vec_sort_cb) compare);
}

static inline int vec_push_create(vec_t ** vec, void * data)
{
    if (*vec)
        return vec_push(vec, data);
    *vec = vec_new(7);
    if (!*vec)
        return -1;
    VEC_push(*vec, data);
    return 0;
}

static inline void vec_swap(vec_t * vec, uint32_t i, uint32_t j)
{
    void * tmp = vec->data[i];
    vec->data[i] = vec->data[j];
    vec->data[j] = tmp;
}

static inline void vec_fill_null(vec_t * vec)
{
    vec->n = vec->sz;
    (void) memset(vec->data, 0, vec->n * sizeof(void*));
}

#endif /* VEC_H_ */
