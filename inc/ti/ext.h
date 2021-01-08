/*
 * ti/ext.h
 */
#ifndef TI_EXT_H_
#define TI_EXT_H_

#include <stdlib.h>

typedef struct ti_ext_s ti_ext_t;

typedef void (*ti_ext_cb)(void * future, void * data);
typedef void (*ti_ext_destroy_cb)(void * data);

struct ti_ext_s
{
    ti_ext_cb cb;
    ti_ext_destroy_cb destroy_cb;   /* may be NULL */
    void * data;
};

static inline void ti_ext_destroy(ti_ext_t * ext)
{
    if (ext && ext->destroy_cb)
        ext->destroy_cb(ext->data);
    free(ext);
}

#endif  /* TI_ADDON_H_ */
