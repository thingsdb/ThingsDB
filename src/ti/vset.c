/*
 * ti/vset.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/thing.inline.h>
#include <ti/val.h>
#include <ti/vset.h>
#include <tiinc.h>
#include <util/logger.h>

ti_vset_t * ti_vset_create(void)
{
    ti_vset_t * vset = malloc(sizeof(ti_vset_t));
    if (!vset)
        return NULL;

    vset->ref = 1;
    vset->tp = TI_VAL_SET;
    vset->flags = 0;
    vset->spec = TI_SPEC_ANY;

    vset->imap = imap_create();
    if (!vset->imap)
    {
        free(vset);
        return NULL;
    }

    return vset;
}

void ti_vset_destroy(ti_vset_t * vset)
{
    if (!vset)
        return;
    imap_destroy(vset->imap, (imap_destroy_cb) ti_val_drop);
    free(vset);
}

typedef struct
{
    msgpack_packer * pk;
    int options;
} vset__walk_to_pk_t;

static inline int vset__walk_to_pk(ti_thing_t * thing, vset__walk_to_pk_t * w)
{
    return ti_thing_to_pk(thing, w->pk, w->options);
}

int ti_vset_to_pk(ti_vset_t * vset, msgpack_packer * pk, int options)
{
    vset__walk_to_pk_t w = {
            .pk = pk,
            .options = options,
    };
    return ((options < 0 && (
                msgpack_pack_map(pk, 1) ||
                mp_pack_strn(pk, TI_KIND_S_SET, 1)
        )) ||
        msgpack_pack_array(pk, vset->imap->n)
    ) ? -1 : imap_walk(vset->imap, (imap_cb) vset__walk_to_pk, &w);
}

int ti_vset_to_list(ti_vset_t ** vsetaddr)
{
    ti_varr_t * list;
    vec_t * vec = imap_vec((*vsetaddr)->imap);
    if (!vec)
        goto failed;

    list = malloc(sizeof(ti_varr_t));
    if (!list)
        goto failed;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = vec->n ? TI_VFLAG_ARR_MHT : 0;
    list->spec = (*vsetaddr)->spec;
    list->vec = vec;

    for (vec_each(list->vec, ti_val_t, val))
        ti_incref(val);

    ti_val_drop((ti_val_t *) *vsetaddr);
    *vsetaddr = (ti_vset_t *) list;

    return 0;

failed:
    free(vec);
    return -1;
}

int ti_vset_to_tuple(ti_vset_t ** vsetaddr)
{
    if (ti_vset_to_list(vsetaddr))
        return -1;
    (*vsetaddr)->flags |= TI_VFLAG_ARR_TUPLE;
    return 0;
}

static int vset__walk_assign(ti_thing_t * t, ti_vset_t * vset)
{
    if (ti_vset_add(vset, t))
    {
        ti_vset_destroy(vset);
        return -1;
    }
    ti_incref(t);
    return 0;
}

int ti_vset_assign(ti_vset_t ** vsetaddr)
{
    ti_vset_t * vset = *vsetaddr;

    if (vset->ref == 1)
        return 0;  /* with only one reference we do not require a copy */

    if (!(vset = ti_vset_create()))
        return -1;

    if (imap_walk(vset->imap, (imap_cb) vset__walk_assign, vset))
        return -1;  /* vset is destroyed if walk has failed */

    ti_decref(*vsetaddr);
    *vsetaddr = vset;

    return 0;
}

/*
 * Increments the reference for each moved value to the set.
 * The return value is <0 in case of an error and `e` will contain the reason,
 * If 0, then the `thing` was already in the set, and 1 if it was added.
 */
int ti_vset_add_val(ti_vset_t * vset, ti_val_t * val, ex_t * e)
{
    if (!ti_val_is_thing(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "cannot add type `%s` to a "TI_VAL_SET_S,
                ti_val_str(val));
        return e->nr;
    }

    if (vset->spec != TI_SPEC_ANY &&
        vset->spec != ((ti_thing_t *) val)->type_id)
    {
        ex_set(e, EX_TYPE_ERROR,
                "type `%s` is not allowed in restricted "TI_VAL_SET_S,
                ti_val_str(val));
        return e->nr;
    }

    switch((imap_err_t) ti_vset_add(vset, (ti_thing_t *) val))
    {
    case IMAP_ERR_EXIST:
        return 0;
    case IMAP_ERR_ALLOC:
        ex_set_mem(e);
        return e->nr;
    case IMAP_SUCCESS:
        ti_incref(val);
    }
    return 1;
}
