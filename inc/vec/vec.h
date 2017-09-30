/*
 * vec.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef RQL_VEC_H_
#define RQL_VEC_H_

typedef struct vec_s  vec_t;

#include <stdio.h>
#include <inttypes.h>


vec_t * vec_create(uint32_t sz);
static inline void vec_destroy(vec_t * vec);
static inline void * vec_get(vec_t * vec, uint32_t i);
static inline void * vec_pop(vec_t * vec);
vec_t * vec_copy(vec_t * vec);
vec_t * vec_append(vec_t * vec, void * data);
vec_t * vec_shrink(vec_t * vec);
vec_t * vec_extend(vec_t * vec, void * data[], uint32_t n);

struct vec_s
{
    uint32_t n;
    uint32_t sz;
    void * data_;
};

static inline void vec_destroy(vec_t * vec)
{
    free(vec);
}

static inline void * vec_get(vec_t * vec, uint32_t i)
{
    return vec->data_[i];
}

static inline void * vec_pop(vec_t * vec)
{
    return (vec->n) ? vec->data_[--vec->n] : NULL;
}

#endif /* RQL_VEC_H_ */
