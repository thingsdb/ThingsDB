/*
 * vec.h
 */
#ifndef VEC_H_
#define VEC_H_

typedef struct vec_s  vec_t;
typedef void (*vec_destroy_cb)(void * data);

#include <stdio.h>
#include <stdint.h>

vec_t * vec_new(uint32_t sz);
void vec_destroy(vec_t * vec, vec_destroy_cb cb);
static inline uint32_t vec_space(const vec_t * vec);
static inline void * vec_get(const vec_t * vec, uint32_t i);
static inline void * vec_pop(vec_t * vec);
static inline void vec_clear(vec_t * vec);
void * vec_remove(vec_t * vec, uint32_t i);
vec_t * vec_dup(const vec_t * vec);
int vec_push(vec_t ** vaddr, void * data);
int vec_extend(vec_t ** vaddr, void * data[], uint32_t n);
int vec_resize(vec_t ** vaddr, uint32_t sz);
int vec_shrink(vec_t ** vaddr);

/* unsafe macro for vec_push() which assumes the vector has enough space */
#define VEC_push(vec__, data_) (vec__)->data[(vec__)->n++] = data_

/* use vec_each in a for loop to go through all values.
 * do not change the vec while iterating over the values */
#define vec_each(vec__, dt__, var__) \
    dt__ * var__, \
    ** v__ = (dt__ **) (vec__)->data, \
    ** e__ = v__ + (vec__)->n; \
    v__ < e__ && ((var__ = *v__) || 1); \
    v__++

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

static inline void * vec_get(const vec_t * vec, uint32_t i)
{
    return vec->data[i];
}

static inline void * vec_pop(vec_t * vec)
{
    return (vec->n) ? vec->data[--vec->n] : NULL;
}

static inline void vec_clear(vec_t * vec)
{
    vec->n = 0;
}

#endif /* VEC_H_ */
