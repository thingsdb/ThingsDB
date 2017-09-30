/*
 * vec.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */


#include <vec/vec.h>

vec_t * vec_create(uint32_t sz)
{
    vec_t * vec = (vec_t *) malloc(sizeof(vec_t) + sz * sizeof(void*));
    if (!vec) return NULL;
    vec->sz = sz;
    vec->n = 0;
    return vec;
}

vec_t * vec_copy(vec_t * vec)
{
    size_t sz = sizeof(vec_t) + vec->sz * sizeof(void*);
    vec_t * vec_ = (vec_t *) malloc(sz);
    if (! vec_) return NULL;
    memcpy( vec_, vec, sz);
    return vec_;
}

vec_t * vec_append(vec_t * vec, void * data)
{
    if (vec->n == vec->sz)
    {
        size_t sz = vec->sz;

        if (sz < 5)
        {
            vec->sz++;
        }
        else if (sz < 64)
        {
            vec->sz *= 2;
        }
        else
        {
            vec->sz += 64;
        }

        vec_t * tmp = (vec_t *) realloc(
                vec,
                sizeof(vec_t) + vec->sz * sizeof(void*));

        if (!tmp)
        {
            /* restore original size */
            vec->sz = sz;
            return NULL;
        }

        vec = tmp;
    }

    vec->data_[vec->n++] = data;

    return vec;
}

vec_t * vec_shrink(vec_t * vec)
{
    if (vec->n == vec->sz) return vec;
    vec_t * tmp = (vec_t *) realloc(
            vec,
            sizeof(vec_t) + vec->n * sizeof(void*));
    if (!tmp) return NULL;
    vec = tmp;
    vec->sz = vec->n;
    return vec;
}
