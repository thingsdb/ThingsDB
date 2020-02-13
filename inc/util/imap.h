/*
 * imap.h
 */
#ifndef IMAP_H_
#define IMAP_H_

typedef enum
{
    IMAP_ERR_EXIST  =-2,
    IMAP_ERR_ALLOC  =-1,
    IMAP_SUCCESS    =0
} imap_err_t;

typedef struct imap_node_s imap_node_t;
typedef struct imap_s imap_t;

typedef int (*imap_cb)(void * data, void * arg);
typedef void (*imap_destroy_cb)(void * data);
typedef void (*imap_update_cb)(
        imap_t * dest,
        imap_t * imap,
        imap_destroy_cb decref_cb);

#include <stdint.h>
#include <stddef.h>
#include <util/vec.h>

imap_t * imap_create(void);
void imap_destroy(imap_t * imap, imap_destroy_cb cb);
void imap_clear(imap_t * imap, imap_destroy_cb cb);
void * imap_set(imap_t * imap, uint64_t id, void * data);
int imap_add(imap_t * imap, uint64_t id, void * data);
void * imap_get(imap_t * imap, uint64_t id);
void * imap_pop(imap_t * imap, uint64_t id);
int imap_walk(imap_t * imap, imap_cb cb, void * arg);
void imap_walkn(imap_t * imap, size_t * n, imap_cb cb, void * arg);
_Bool imap__eq_(imap_t * a, imap_t * b);
static inline _Bool imap_eq(imap_t * a, imap_t * b);
vec_t * imap_vec(imap_t * imap);
uint64_t imap_unused_id(imap_t * imap, uint64_t max);
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
    imap_node_t nodes[];
};

static inline _Bool imap_eq(imap_t * a, imap_t * b)
{
    return a == b || (a->n == b->n && (!a->n || imap__eq_(a, b)));
}

#endif /* IMAP_H_ */
