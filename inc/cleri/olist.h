/*
 * olist.h - linked list for keeping cleri objects.
 */
#ifndef CLERI_OLIST_H_
#define CLERI_OLIST_H_

#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_olist_s cleri_olist_t;
typedef struct cleri_olist_s cleri_olist_t;

/* private functions */
cleri_olist_t * cleri__olist_new(void);
int cleri__olist_append(cleri_olist_t * olist, cleri_t * cl_object);
int cleri__olist_append_nref(cleri_olist_t * olist, cleri_t * cl_object);
void cleri__olist_free(cleri_olist_t * olist);
void cleri__olist_empty(cleri_olist_t * olist);
void cleri__olist_cancel(cleri_olist_t * olist);
void cleri__olist_unique(cleri_olist_t * olist);

/* structs */
struct cleri_olist_s
{
    cleri_t * cl_obj;
    cleri_olist_t * next;
};

#endif /* CLERI_OLIST_H_ */