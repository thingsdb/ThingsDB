/*
 * util/omap.h
 */
#ifndef OMAP_H_
#define OMAP_H_

enum
{
    OMAP_ERR_EXIST  =-2,
    OMAP_ERR_ALLOC  =-1,
    OMAP_SUCCESS    =0
};

typedef struct omap_s omap_t;

#include <inttypes.h>

typedef void (*omap_destroy_cb)(void * data);

/* private */
struct omap__s
{
    struct omap__s * next_;
    uint64_t id;
    void * data_;
};

omap_t * omap_create(void);
void omap_destroy(omap_t * omap, omap_destroy_cb cb);
int omap_add(omap_t * omap, uint64_t id, void * data);
void * omap_set(omap_t * omap, uint64_t id, void * data);
void * omap_get(omap_t * omap, uint64_t id);
void * omap_rm(omap_t * omap, uint64_t id);

struct omap_s
{
    struct omap__s * next_;
    size_t n;
};

#endif  /* OMAP_H_ */
