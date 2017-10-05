/*
 * vec.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef VEC_H_
#define VEC_H_

typedef struct vec_s  vec_t;
typedef void (*vec_destroy_cb)(void * data);

#include <stdio.h>
#include <inttypes.h>


vec_t * vec_create(uint32_t sz);
void vec_destroy(vec_t * vec, vec_destroy_cb cb);
static inline void * vec_get(vec_t * vec, uint32_t i);
static inline void * vec_pop(vec_t * vec);
vec_t * vec_copy(vec_t * vec);
vec_t * vec_append(vec_t * vec, void * data);
vec_t * vec_shrink(vec_t * vec);
vec_t * vec_extend(vec_t * vec, void * data[], uint32_t n);

/* unsafe macro for vec_append() which assumes the vector has enough space */
#define VEC_append(vec__, data__) (vec__)->data_[(vec__)->n++] = data__

struct vec_s
{
    uint32_t n;
    uint32_t sz;
    void * data_[];
};

static inline void * vec_get(vec_t * vec, uint32_t i)
{
    return vec->data_[i];
}

static inline void * vec_pop(vec_t * vec)
{
    return (vec->n) ? vec->data_[--vec->n] : NULL;
}


#endif /* VEC_H_ */
