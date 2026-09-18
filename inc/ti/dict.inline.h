/*
 * ti/dict.inline.h
 */
#ifndef TI_DICT_INLINE_H_
#define TI_DICT_INLINE_H_

#include <ti/dict.h>
#include <ti/val.h>
#include <ex.h>

static inline _Bool ti_dict_has(ti_dict_t * dict, ti_val_t * key)
{
    return ti_dict_get_weak(dict, key) != NULL;
}

static inline _Bool ti_dict_get(ti_dict_t * dict, ti_val_t * key)
{
    ti_val_t * val = ti_dict_get_weak(dict, key) != NULL;
    ti_incref(val);
    return val;
}

static inline _Bool ti_dict_is_stored(ti_dict_t * dict)
{
    return dict->parent && dict->parent->id;
}

static inline int ti_dict_set(ti_dict_t * dict, ti_val_t * key, ti_val_t * val, ex_t * e)

#endif  /* TI_DICT_INLINE_H_ */
