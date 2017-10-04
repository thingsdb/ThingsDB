/*
 * imap.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef IMAP_H_
#define IMAP_H_

#define IMAP_ERR_ALLOC -1
#define IMAP_ERR_EXIST -2

typedef struct imap_node_s imap_node_t;
typedef struct imap_s imap_t;

typedef int (*imap_cb)(void * data, void * arg);
typedef void (*imap_destroy_cb)(void * data);
typedef void (*imap_update_cb)(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);

#include <inttypes.h>
#include <stddef.h>
#include <vec/vec.h>

imap_t * imap_create(void);
void imap_destroy(imap_t * imap, imap_destroy_cb cb);
void * imap_set(imap_t * imap, uint64_t id, void * data);
int imap_add(imap_t * imap, uint64_t id, void * data);
void * imap_get(imap_t * imap, uint64_t id);
void * imap_pop(imap_t * imap, uint64_t id);
int imap_walk(imap_t * imap, imap_cb cb, void * arg);
void imap_walkn(imap_t * imap, size_t * n, imap_cb cb, void * arg);
vec_t * imap_vec(imap_t * imap);
vec_t * imap_vec_pop(imap_t * imap);
void imap_union_ref(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);
void imap_intersection_ref(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);
void imap_difference_ref(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);
void imap_symmetric_difference_ref(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);

struct imap_node_s
{
    size_t sz;
    void * data;
    imap_node_t * nodes;
};

struct imap_s
{
    size_t n;
    vec_t * vec;
    imap_node_t nodes[];
};


#endif /* IMAP_H_ */
