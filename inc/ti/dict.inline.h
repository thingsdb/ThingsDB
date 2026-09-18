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

static inline void * ti_vset_key(ti_vset_t * vset)
{
    return ti_thing_is_object(vset->parent)
            ? vset->key_
            : ((ti_field_t *) vset->key_)->name;
}


static inline int ti_dict_set(ti_dict_t * dict,
                              ti_val_t * key,
                              ti_val_t * val,
                              ex_t * e)
{
    switch((ti_val_enum) key->tp)
    {
        case TI_VAL_UUID:
            if (ti_dict_set_uuid(dict, VUUID(key), val))
                ex_set_mem(e);
            break;
        case TI_VAL_INT:
            if (ti_dict_set_int(dict, VINT(key), val))
                ex_set_mem(e);
            break;
        case TI_VAL_NAME:
        case TI_VAL_STR:
        {
            ti_str_t * str = (ti_str_t *) key;
            if (ti_dict_set_strn(dict, str->str, str->n, val))
                ex_set_mem(e);
            break;
        }
        default:
            ex_set(e, EX_TYPE_ERROR,
                "cannot use type `%s` as a dictionary key",
                ti_val_str(key));
    }
    return e->nr;
}

#endif  /* TI_DICT_INLINE_H_ */
