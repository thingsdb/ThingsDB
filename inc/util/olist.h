/*
 * util/olist.h
 */
#ifndef OLIST_H_
#define OLIST_H_

typedef struct olist_s olist_t;
typedef struct olist__s olist__t;
typedef struct olist__s * olist_iter_t;

#include <inttypes.h>

/* private */
struct olist__s
{
    olist__t * next_;
    uint64_t id_;
};

olist_t * olist_create(void);
void olist_destroy(olist_t * olist);
uint64_t * olist_last_id(olist_t * olist);
int olist_set(olist_t * olist, uint64_t id);
_Bool olist_is_set(olist_t * olist, uint64_t id);
void olist_rm(olist_t * olist, uint64_t id);
static inline olist_iter_t olist_iter(olist_t * olist);
static inline uint64_t olist_iter_id(olist_iter_t iter);
#define olist_each(iter__, dt__, var__) \
        dt__ * var__; \
        iter__ && \
        (var__ = (dt__ *) iter__->data_); \
        iter__ = iter__->next_

struct olist_s
{
    olist__t * next_;
    size_t n;
};

static inline olist_iter_t olist_iter(olist_t * olist)
{
    return olist->next_;
}

static inline uint64_t olist_iter_id(olist_iter_t iter)
{
    return iter->id_;
}

#endif  /* OLIST_H_ */
