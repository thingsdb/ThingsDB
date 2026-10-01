/*
 * ti/dict.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/dict.h>
#include <ti/dict.inline.h>
#include <ti/dict.t.h>
#include <ti/raw.inline.h>
#include <ti/spec.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <tiinc.h>
#include <util/logger.h>
#include <util/vec.h>

#define DICT__MAX_STACK_SIZE 512

ti_dict_t * ti_dict_create(void)
{
    ti_dict_t * dict = malloc(sizeof(ti_dict_t));
    if (!dict)
        return NULL;

    dict->ref = 1;
    dict->tp = TI_VAL_DICT;
    dict->flags = 0;

    dict->imap_ = NULL;
    dict->smap_ = NULL;
    dict->umap_ = NULL;

    dict->parent = NULL;

    return dict;
}

void ti_dict_destroy(ti_dict_t * dict)
{
    imap_destroy(dict->imap_, (imap_destroy_cb) ti_val_unsafe_gc_drop);
    smap_destroy(dict->smap_, (smap_destroy_cb) ti_val_unsafe_gc_drop);
    umap_destroy(dict->umap_, (umap_destroy_cb) ti_val_unsafe_gc_drop);
    free(dict);
}

void ti_dict_clear(ti_dict_t * dict)
{
    imap_destroy(dict->imap_, (imap_destroy_cb) ti_val_unsafe_gc_drop);
    smap_destroy(dict->smap_, (smap_destroy_cb) ti_val_unsafe_gc_drop);
    umap_destroy(dict->umap_, (umap_destroy_cb) ti_val_unsafe_gc_drop);
    dict->imap_ = NULL;
    dict->smap_ = NULL;
    dict->umap_ = NULL;
}

typedef struct
{
    ti_dict_pair_cb cb;
    void * arg;
} dict__pair_t;

typedef struct
{
    ti_dict_item_cb cb;
    void * arg;
} dict__item_t;

static int dict__umap_item(const uuid_t uuid, ti_val_t * val, dict__item_t * w)
{
    ti_val_t * key = (ti_val_t *) ti_uuid_from_bytes(uuid);
    if (!key)
        return -1;
    int rc = w->cb(key, val, w->arg);
    ti_val_unsafe_drop(key);
    return rc;
}

static int dict__imap_item(uint64_t id, ti_val_t * val, dict__item_t * w)
{
    ti_val_t * key = (ti_val_t *) ti_vint_create((int64_t) id);
    if (!key)
        return -1;
    int rc = w->cb(key, val, w->arg);
    ti_val_unsafe_drop(key);
    return rc;
}

static int dict__smap_item(
        const char * str,
        size_t n,
        ti_val_t * val,
        dict__item_t * w)
{
    ti_val_t * key = (ti_val_t *) ti_str_create(str, n);
    if (!key)
        return -1;
    int rc = w->cb(key, val, w->arg);
    ti_val_unsafe_drop(key);
    return rc;
}

static int dict__umap_pair(uuid_t uuid, ti_val_t * val, dict__pair_t * w)
{
    ti_dict_key_t key = {
        .tp = TI_DICT_KEY_UUID,
    };
    memcpy(key.via.uuid, uuid, sizeof(uuid_t));
    return w->cb(&key, val, w->arg);
}

static int dict__imap_pair(uint64_t id, ti_val_t * val, dict__pair_t * w)
{
    ti_dict_key_t key = {
        .tp = TI_DICT_KEY_INT,
        .via.id = (int64_t) id,
    };
    return w->cb(&key, val, w->arg);
}

static int dict__smap_pair(
        const char * str,
        size_t n,
        ti_val_t * val,
        dict__pair_t * w)
{
    ti_dict_key_t key = {
        .tp = TI_DICT_KEY_STR,
        .via.str.str = str,
        .via.str.n = n,
    };
    return w->cb(&key, val, w->arg);
}

/*
 * Run the call-back function on all items in the dict.
 *
 * Walking stops on the first callback returning a non zero value OR when a
 * memory allocation error has occurred (-1) (can happen as values
 * need to be created from keys).
 * Otherwise, the return value is the last callback result. A return value
 * of 0 means that the callback function is called on all items in the dict.
 */
int ti_dict_items(ti_dict_t * dict, ti_dict_item_cb cb, void * arg)
{
    int rc = 0;
    dict__item_t w = {
        .arg = arg,
        .cb = cb,
    };

    if (dict->umap_ &&
        dict->umap_->n &&
        (rc = umap_items(dict->umap_, (umap_item_cb) dict__umap_item, &w)))
        return rc;

    if (dict->imap_ &&
        dict->imap_->n &&
        (rc = imap_items(dict->imap_, (imap_item_cb) dict__imap_item, &w)))
        return rc;

    if (dict->smap_ && dict->smap_->n)
    {
        char stack_buf[DICT__MAX_STACK_SIZE];
        char * buf = stack_buf;
        size_t max_key_size = smap_longest_key_size(dict->smap_);

        if (max_key_size > DICT__MAX_STACK_SIZE)
        {
            buf = (char *) malloc(max_key_size);
            if (!buf)
                return -1;
        }

        rc = smap_items(dict->smap_,
                        buf,
                        (smap_item_cb) dict__smap_item,
                        &w);

        if (buf != stack_buf)
            free(buf);

        return rc;
    }
    return rc;
}

/*
 * Run the call-back function on all values in the dict.
 *
 * Walking stops on the first callback returning a non zero value.
 * The return value is the last callback result. A return value
 * of 0 means that the callback function is called on all items in the dict.
 *
 * This function can return -1 in case not enough space for a string buffer
 * can be allocated. This can only happen with keys exceeding the
 * reserverd DICT__MAX_STACK_SIZE size.
 */
int ti_dict_pairs(ti_dict_t * dict, ti_dict_pair_cb cb, void * arg)
{
    int rc = 0;
    dict__pair_t w = {
        .arg = arg,
        .cb = cb,
    };

    if (dict->umap_ &&
        dict->umap_->n &&
        (rc = umap_items(dict->umap_, (umap_item_cb) dict__umap_pair, &w)))
        return rc;

    if (dict->imap_ &&
        dict->imap_->n &&
        (rc = imap_items(dict->imap_, (imap_item_cb) dict__imap_pair, &w)))
        return rc;

    if (dict->smap_ && dict->smap_->n)
    {
        char stack_buf[DICT__MAX_STACK_SIZE];
        char * buf = stack_buf;
        size_t max_key_size = smap_longest_key_size(dict->smap_);

        if (max_key_size > DICT__MAX_STACK_SIZE)
        {
            buf = (char *) malloc(max_key_size);
            if (!buf)
                return -1;
        }

        rc = smap_items(dict->smap_,
                        buf,
                        (smap_item_cb) dict__smap_pair,
                        &w);

        if (buf != stack_buf)
            free(buf);
    }
    return rc;
}

/*
 * Run the call-back function on all values in the dict.
 *
 * Walking stops on the first callback returning a non zero value.
 * The return value is the last callback result. A return value
 * of 0 means that the callback function is called on all items in the dict.
 */
int ti_dict_walk(ti_dict_t * dict, ti_dict_cb cb, void * arg)
{
    int rc = 0;
    if (dict->umap_ &&
        dict->umap_->n &&
        (rc = umap_walk(dict->umap_, (umap_cb) cb, arg)))
        return rc;

    if (dict->imap_ &&
        dict->imap_->n &&
        (rc = imap_walk(dict->imap_, (imap_cb) cb, arg)))
        return rc;

    if (dict->smap_ &&
        dict->smap_->n &&
        (rc = smap_values(dict->smap_, (smap_val_cb) cb, arg)))
        return rc;

    return rc;
}

typedef struct
{
    ti_vp_t * vp;
    int deep;
    int flags;
} dict__client_pk_t;

static int dict__pair_client_pk_cb(ti_dict_key_t * key,
                                   ti_val_t * val,
                                   dict__client_pk_t * w)
{
    return -(
        msgpack_pack_array(&w->vp->pk, 2) ||
        ti_dict_key_to_client_pk(key, &w->vp->pk) ||
        ti_val_to_client_pk(val, w->vp, w->deep, w->flags)
    );
}

static int dict__pair_store_pk_cb(ti_dict_key_t * key,
                                  ti_val_t * val,
                                  msgpack_packer * pk)
{
    switch(key->tp)
    {
        case TI_DICT_KEY_UUID:
            if (ti_uuid_to_store_pk(key->via.uuid, pk))
                return -1;
            break;
        case TI_DICT_KEY_INT:
            if (msgpack_pack_int64(pk, key->via.id))
                return -1;
            break;
        case TI_DICT_KEY_STR:
            if (mp_pack_strn(pk, key->via.str.str, key->via.str.n))
                return -1;
            break;
    }
    return ti_val_to_store_pk(val, pk);
}

int ti_dict_to_client_pk(ti_dict_t * dict, ti_vp_t * vp, int deep, int flags)
{
    dict__client_pk_t w = {
        .vp = vp,
        .deep = deep,
        .flags = flags,
    };
    return -(
        msgpack_pack_array(&vp->pk, ti_dict_n(dict)) ||
        ti_dict_pairs(dict, (ti_dict_pair_cb) dict__pair_client_pk_cb, &w)
    );
}

int ti_dict_to_store_pk(ti_dict_t * dict, msgpack_packer * pk)
{
    return -(
        msgpack_pack_map(pk, 1) ||
        mp_pack_strn(pk, TI_KIND_S_DICT, 1) ||
        msgpack_pack_map(pk, ti_dict_n(dict)) ||
        ti_dict_pairs(dict, (ti_dict_pair_cb) dict__pair_store_pk_cb, pk)
    );
}

static int dict__as_vec_cb(ti_val_t * key, ti_val_t * val, vec_t * vec)
{
    ti_tuple_t * tuple = malloc(sizeof(ti_tuple_t));
    if (!tuple)
        goto failed;

    tuple->vec = vec_new(2);
    if (!tuple->vec)
        goto failed;

    tuple->ref = 1;
    tuple->tp = TI_VAL_ARR;
    tuple->flags = TI_VARR_FLAG_TUPLE | (
        ti_val_is_thing(val) ? TI_VFLAG_MHT :
        ti_val_is_room(val) ? TI_VFLAG_MHR : 0);

    ti_incref(key);
    ti_incref(val);
    VEC_push(tuple->vec, key);
    VEC_push(tuple->vec, val);
    VEC_push(vec, tuple);
    return 0;

failed:
    free(tuple);
    return -1;
}

static vec_t * dict__as_vec(ti_dict_t * dict)
{
    vec_t * vec = vec_new(ti_dict_n(dict));
    if (vec && ti_dict_items(dict, (ti_dict_item_cb) dict__as_vec_cb, vec))
    {
        vec_destroy(vec, (vec_destroy_cb) ti_val_unsafe_drop);
        return NULL;
    }
    return vec;
}

int ti_dict_to_list(ti_dict_t ** dictaddr)
{
    ti_dict_t * dict = (ti_dict_t *) (*dictaddr);
    ti_varr_t * list = malloc(sizeof(ti_varr_t));
    if (!list)
        goto failed;

    list->vec = dict__as_vec(dict);
    if (!list->vec)
        goto failed;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = ti_val_may_flags(dict);
    list->parent = NULL;

    ti_val_unsafe_drop((ti_val_t *) *dictaddr);
    *dictaddr = (ti_dict_t *) list;

    return 0;

failed:
    free(list);
    return -1;
}

int ti_dict_to_tuple(ti_dict_t ** dictaddr)
{
    ti_dict_t * dict = (ti_dict_t *) (*dictaddr);
    ti_tuple_t * tuple = malloc(sizeof(ti_tuple_t));
    if (!tuple)
        goto failed;

    tuple->vec = dict__as_vec(dict);
    if (!tuple->vec)
        goto failed;

    tuple->ref = 1;
    tuple->tp = TI_VAL_ARR;
    tuple->flags = TI_VARR_FLAG_TUPLE | ti_val_may_flags(dict);

    ti_val_unsafe_drop((ti_val_t *) *dictaddr);
    *dictaddr = (ti_dict_t *) tuple;

    return 0;

failed:
    free(tuple);
    return -1;
}

static int dict__assign_cb(ti_dict_key_t * key, ti_val_t * val, ti_dict_t * dict)
{
    if (ti_dict_setr(dict, key, val))
    {
        ti_dict_destroy(dict);
        return -1;
    }
    return 0;
}

ti_dict_t * ti_dict_cp(ti_dict_t * dict)
{
    ti_dict_t * ndict = ti_dict_create();
    if (!ndict)
        return NULL;

    ndict->flags = ti_val_may_flags(dict);

    return ti_dict_pairs(dict, (ti_dict_pair_cb) dict__assign_cb, ndict)
            ? NULL  /* ndict is destroyed if walk has failed */
            : ndict;
}

ti_val_t * ti_dict_del(ti_dict_t * dict, ti_val_t * key)
{
    switch(key->tp)
    {
        case TI_VAL_UUID:
            return dict->umap_ ? umap_pop(dict->umap_, VUUID(key)) : NULL;
        case TI_VAL_INT:
            return dict->imap_ ? imap_pop(dict->imap_, VINT(key)) : NULL;
        case TI_VAL_STR:
        case TI_VAL_NAME:
            return dict->smap_
                ? smap_popn(dict->smap_,
                            ((ti_str_t *) key)->str,
                            ((ti_str_t *) key)->n)
                : NULL;
    }
    return NULL;
}

int ti_dict_assign(ti_dict_t ** dictaddr)
{
    ti_dict_t * ndict, * odict = *dictaddr;

    if (odict->ref == 1)
        return 0;  /* with only one reference we do not require a copy */

    if (!(ndict = ti_dict_create()))
        return -1;

    ndict->flags = ti_val_may_flags(odict);

    if (ti_dict_pairs(odict, (ti_dict_pair_cb) dict__assign_cb, ndict))
        return -1;  /* ndict is destroyed if walk has failed */

    ti_decref(odict);  /* checked for more than one reference */
    *dictaddr = ndict;

    return 0;
}

typedef struct
{
    ti_dict_t * dict;
    int deep;
} dict__copy_t;

static int dict__copy_cb(ti_dict_key_t * key, ti_val_t * val, dict__copy_t * w)
{
    int rc;
    ti_val_t ** vaddr = &val;
    ti_incref(val);
    rc = (ti_val_copy_nested(vaddr, w->deep) ||
          ti_dict_setr(w->dict, key, *vaddr));
    ti_decref(val);

    if (rc)
    {
        ti_dict_destroy(w->dict);
        return -1;
    }
    return 0;
}

int ti_dict_copy(ti_dict_t ** dictaddr, uint8_t deep)
{
    assert(deep);
    ti_dict_t * ndict, * odict = *dictaddr;

    if (!(ndict = ti_dict_create()))
        return -1;

    dict__copy_t w = {
        .dict = ndict,
        .deep = deep,
    };

    if (ti_dict_pairs(odict, (ti_dict_pair_cb) dict__copy_cb, &w))
        return -1;  /* vset is destroyed if walk has failed */

    /* set default flags */
    ndict->flags = ti_val_may_flags(odict);

    ti_val_unsafe_drop((ti_val_t *) odict);
    *dictaddr = ndict;

    return 0;
}

static int dict__dup_cb(ti_dict_key_t * key, ti_val_t * val, dict__copy_t * w)
{
    int rc;
    ti_val_t ** vaddr = &val;
    ti_incref(val);
    rc = (ti_val_dup_nested(vaddr, w->deep) ||
          ti_dict_setr(w->dict, key, *vaddr));
    ti_decref(val);

    if (rc)
    {
        ti_dict_destroy(w->dict);
        return -1;
    }
    return 0;
}

int ti_dict_dup(ti_dict_t ** dictaddr, uint8_t deep)
{
    assert(deep);

    ti_dict_t * ndict, * odict = *dictaddr;

    if (!(ndict = ti_dict_create()))
        return -1;

    dict__copy_t w = {
        .dict = ndict,
        .deep = deep,
    };

    if (ti_dict_pairs(odict, (ti_dict_pair_cb) dict__dup_cb, &w))
        return -1;  /* vset is destroyed if walk has failed */

    /* set default flags */
    ndict->flags = ti_val_may_flags(odict);

    ti_val_unsafe_drop((ti_val_t *) odict);
    *dictaddr = ndict;

    return 0;
}

/*
 * Increases the reference counter of `val` on newly written key/pair.
 * New value is created for type list, set and dict as they cannot be nested.
 */
int ti_dict_set(ti_dict_t * dict, ti_val_t * key, ti_val_t ** val, ex_t * e)
{
    switch (ti_dict_key_spec(dict))
    {
        case TI_SPEC_ANY:
            break;
        case TI_SPEC_UUID:
            if (!ti_val_is_uuid(key))
            {
                ex_set(e, EX_TYPE_ERROR,
                    "dict key must be of type `"TI_VAL_UUID_S"`, "
                    "got `%s`",
                    ti_val_str(key));
                return e->nr;
            }
            break;
        case TI_SPEC_INT:
            if (!ti_val_is_int(key))
            {
                ex_set(e, EX_TYPE_ERROR,
                    "dict key must be of type `"TI_VAL_INT_S"`, "
                    "got `%s`",
                    ti_val_str(key));
                return e->nr;
            }
            break;
        case TI_SPEC_STR:
            if (!ti_val_is_str(key))
            {
                ex_set(e, EX_TYPE_ERROR,
                    "dict key must be of type `"TI_VAL_STR_S"`, "
                    "got `%s`",
                    ti_val_str(key));
                return e->nr;
            }
            break;
    }

    switch (ti_spec_check_nested_val(ti_dict_val_spec(dict), *val))
    {
        case TI_SPEC_RVAL_SUCCESS:
            break;
        case TI_SPEC_RVAL_TYPE_ERROR:
            {
                /* TYPE_ERROR can only occur when this is an instance */
                assert(dict->parent);
                assert(ti_thing_is_instance(dict->parent));
                assert(dict->key_);
                ti_field_t * field = (ti_field_t *) dict->key_;
                ex_set(e, EX_TYPE_ERROR,
                    "dict requires values to match definition `%.*s`, "
                    "got type `%s`",
                    field->spec_raw->n, (const char *) field->spec_raw->data,
                    ti_val_str(*val));
            }
            return e->nr;
        case TI_SPEC_RVAL_UTF8_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be valid UTF8 strings");
            return e->nr;
        case TI_SPEC_RVAL_UINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be integers greater than or equal to 0");
            return e->nr;
        case TI_SPEC_RVAL_PINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be positive integers");
            return e->nr;
        case TI_SPEC_RVAL_NINT_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be negative integers");
            return e->nr;
        case TI_SPEC_RVAL_EMAIL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be email addresses");
            return e->nr;
        case TI_SPEC_RVAL_URL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be URL's");
            return e->nr;
        case TI_SPEC_RVAL_TEL_ERROR:
            ex_set(e, EX_VALUE_ERROR,
                "dict requires values to be telephone numbers");
            return e->nr;
    }

    if (ti_val(*val)->to_nested(val, (ti_parent_t *) dict, e))
        return e->nr;

    switch((ti_val_enum) key->tp)
    {
        case TI_VAL_UUID:
            if (ti_dict_set_uuid(dict, VUUID(key), *val))
                goto memerr;
            break;
        case TI_VAL_INT:
            if (ti_dict_set_int(dict, VINT(key), *val))
                goto memerr;
            break;
        case TI_VAL_NAME:
        case TI_VAL_STR:
        {
            ti_str_t * str = (ti_str_t *) key;
            if (ti_dict_set_strn(dict, str->str, str->n, *val))
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