/*
 * ti/name.h
 */
#ifndef TI_NAME_H_
#define TI_NAME_H_

typedef struct ti_name_s ti_name_t;

#include <stdint.h>
#include <stdlib.h>

ti_name_t * ti_name_create(const char * str, size_t n);
void ti_name_drop(ti_name_t * name);
_Bool * ti_name_is_valid_strn(const char * str, size_t n);
static inline void ti_name_destroy(ti_name_t * name);


struct ti_name_s
{
    uint32_t ref;
    uint32_t n;             /* strlen(name->name) */
    char str[];            /* null terminated string */
};

static inline void ti_name_destroy(ti_name_t * name)
{
    free(name);
}

#endif /* TI_NAME_H_ */
