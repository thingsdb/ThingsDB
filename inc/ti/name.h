/*
 * ti/name.h
 */
#ifndef TI_NAME_H_
#define TI_NAME_H_

typedef struct ti_name_s ti_name_t;

#include <stdint.h>
#include <stdlib.h>
#include <ti/val.h>

ti_name_t * ti_name_create(const char * str, size_t n);
void ti_name_destroy(ti_name_t * name);
_Bool ti_name_is_valid_strn(const char * str, size_t n);
static inline void ti_name_drop(ti_name_t * name);

/*
 * name can be cast to raw. the difference is in `char str[]`, but the other
 * fields map to `raw`. therefore the `tp` of name should be equal to `raw`
 */
struct ti_name_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    uint32_t n;
    char str[];             /* null terminated string */
};


static inline void ti_name_drop(ti_name_t * name)
{
    if (name && !--name->ref)
        ti_name_destroy(name);
}

#endif /* TI_NAME_H_ */
