/*
 * ti/dict.inline.h
 */
#ifndef TI_DICT_INLINE_H_
#define TI_DICT_INLINE_H_

#include <ti/dict.h>
#include <ti/val.h>
#include <ti/val.t.h>
#include <ti/val.inline.h>
#include <ti/thing.inline.h>
#include <ex.h>

static inline ti_val_t * ti_dict_get_weak(ti_dict_t * dict, ti_val_t * key)
{
    switch(key->tp)
    {
        case TI_VAL_UUID:
            return umap_get(dict->umap_, VUUID(key));
        case TI_VAL_NAME:
        case TI_VAL_STR:
            return smap_getn(dict->smap_,
                             ((ti_str_t *) key)->str,
                             ((ti_str_t *) key)->n);
        case TI_VAL_INT:
            return imap_get(dict->imap_, VINT(key));
    }
    return NULL;
}

static inline _Bool ti_dict_has(ti_dict_t * dict, ti_val_t * key)
{
    return ti_dict_get_weak(dict, key) != NULL;
}

static inline _Bool ti_dict_is_stored(ti_dict_t * dict)
{
    return dict->parent && dict->parent->id;
}

static inline void * ti_dict_key(ti_dict_t * dict)
{
    return ti_thing_is_object(dict->parent)
            ? dict->key_
            : ((ti_field_t *) dict->key_)->name;
}

static inline int ti_dict_set_uuid(ti_dict_t * dict,
                                   uuid_t key,
                                   ti_val_t * val)
{
    if (!dict->umap_ && !(dict->umap_ = umap_create()))
        return -1;
    int new = 0;
    ti_val_t * ret = umap_set(dict->umap_, key, val, &new);
    if (!ret)
        return -1;
    if (ret != val)
        ti_val_unsafe_gc_drop(ret);
    else if (new)
        ti_incref(val);
    return 0;
}

static inline int ti_dict_set_int(ti_dict_t * dict,
                                  int64_t key,
                                  ti_val_t * val)
{
    if (!dict->imap_ && !(dict->imap_ = imap_create()))
        return -1;
    int new = 0;
    ti_val_t * ret = imap_set(dict->imap_, key, val, &new);
    if (!ret)
        return -1;
    if (ret != val)
        ti_val_unsafe_gc_drop(ret);
    else if (new)
        ti_incref(val);
    return 0;
}

static inline int ti_dict_set_strn(ti_dict_t * dict,
                                   const char * key,
                                   size_t n,
                                   ti_val_t * val)
{
    if (!dict->smap_ && !(dict->smap_ = smap_create()))
        return -1;
    int new = 0;
    ti_val_t * ret = smap_setn(dict->smap_, key, n, val, &new);
    if (!ret)
        return -1;
    if (ret != val)
        ti_val_unsafe_gc_drop(ret);
    else if (new)
        ti_incref(val);
    return 0;
}

/*
 * Increases the reference counter of `val` on newly written key/pair.
 */
static inline int ti_dict_setr(ti_dict_t * dict,
                               ti_dict_key_t * key,
                               ti_val_t * val)
{
    switch(key->tp)
    {
    case TI_DICT_KEY_UUID: return ti_dict_set_uuid(dict, key->via.uuid, val);
    case TI_DICT_KEY_INT: return ti_dict_set_int(dict, key->via.id, val);
    case TI_DICT_KEY_STR: return ti_dict_set_strn(dict,
                                                  key->via.str.str,
                                                  key->via.str.n,
                                                  val);
    }
    assert(0);
    return -1;
}

/*
 * Increases the reference counter of `val` on newly written key/pair.
 */
static inline int ti_dict_set(ti_dict_t * dict,
                              ti_val_t * key,
                              ti_val_t * val,
                              ex_t * e)
{
    switch((ti_val_enum) key->tp)
    {
        case TI_VAL_UUID:
            if (ti_dict_set_uuid(dict, VUUID(key), val))
                goto memerr;
            break;
        case TI_VAL_INT:
            if (ti_dict_set_int(dict, VINT(key), val))
                goto memerr;
            break;
        case TI_VAL_NAME:
        case TI_VAL_STR:
        {
            ti_str_t * str = (ti_str_t *) key;
            if (ti_dict_set_strn(dict, str->str, str->n, val))
                goto memerr;
            break;
        }
        default:
            ex_set(e, EX_TYPE_ERROR,
                "cannot use type `%s` as a dictionary key",
                ti_val_str(key));
            return e->nr;
    }
    return e->nr;
memerr:
    ex_set_mem(e);
    return e->nr;
}

static inline uint16_t ti_dict_value_spec(ti_dict_t * dict)
{
    return ((!dict->parent) || ti_thing_is_object(dict->parent))
            ? TI_SPEC_ANY
            : ((ti_field_t *) dict->key_)->nested_spec;
}

static inline int ti_dict_key_to_client_pk(ti_dict_key_t * key,
                                           msgpack_packer * pk)
{
    switch(key->tp)
    {
        case TI_DICT_KEY_UUID:
            return ti_uuid_to_client_pk(key->via.uuid, pk);
        case TI_DICT_KEY_INT:
            return msgpack_pack_int64(pk, key->via.id);
        case TI_DICT_KEY_STR:
            return mp_pack_strn(pk, key->via.str.str, key->via.str.n);
    }
    assert(0);
    return -1;
}

#endif  /* TI_DICT_INLINE_H_ */
