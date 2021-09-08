/*
 * ti/vset.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/thing.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
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
    vset->parent = NULL;

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
    imap_destroy(vset->imap, (imap_destroy_cb) ti_val_unsafe_gc_drop);
    free(vset);
}

typedef struct
{
    ti_vp_t * vp;
    int options;
} vset__walk_to_pk_t;

static inline int vset__walk_to_pk(ti_thing_t * thing, vset__walk_to_pk_t * w)
{
    return ti_thing_to_pk(thing, w->vp, w->options);
}

int ti_vset_to_pk(ti_vset_t * vset, ti_vp_t * vp, int options)
{
    vset__walk_to_pk_t w = {
            .vp = vp,
            .options = options,
    };
    /*
     * Pack as a `map` when options < 0, otherwise pack the set as an array.
     */
    return ((options < 0 && (
                msgpack_pack_map(&vp->pk, 1) ||
                mp_pack_strn(&vp->pk, TI_KIND_S_SET, 1)
        )) ||
        msgpack_pack_array(&vp->pk, vset->imap->n)
    ) ? -1 : imap_walk(vset->imap, (imap_cb) vset__walk_to_pk, &w);
}

int ti_vset_to_list(ti_vset_t ** vsetaddr)
{
    ti_varr_t * list = malloc(sizeof(ti_varr_t));
    if (!list)
        goto failed;

    list->vec = imap_vec_ref((*vsetaddr)->imap);
    if (!list->vec)
        goto failed;

    list->ref = 1;
    list->tp = TI_VAL_ARR;
    list->flags = list->vec->n ? TI_VARR_FLAG_MHT : 0;
    list->parent = NULL;

    ti_val_unsafe_drop((ti_val_t *) *vsetaddr);
    *vsetaddr = (ti_vset_t *) list;

    return 0;

failed:
    free(list);
    return -1;
}

int ti_vset_to_tuple(ti_vset_t ** vsetaddr)
{
    ti_tuple_t * tuple = malloc(sizeof(ti_tuple_t));
    if (!tuple)
        goto failed;

    tuple->vec = imap_vec_ref((*vsetaddr)->imap);
    if (!tuple->vec)
        goto failed;

    tuple->ref = 1;
    tuple->tp = TI_VAL_ARR;
    tuple->flags = TI_VARR_FLAG_TUPLE | (tuple->vec->n ? TI_VARR_FLAG_MHT : 0);

    ti_val_unsafe_drop((ti_val_t *) *vsetaddr);
    *vsetaddr = (ti_vset_t *) tuple;

    return 0;

failed:
    free(tuple);
    return -1;
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
    ti_vset_t * nvset, * ovset = *vsetaddr;

    if (ovset->ref == 1)
        return 0;  /* with only one reference we do not require a copy */

    if (!(nvset = ti_vset_create()))
        return -1;

    if (imap_walk(ovset->imap, (imap_cb) vset__walk_assign, nvset))
        return -1;  /* vset is destroyed if walk has failed */

    ti_decref(ovset);  /* checked for more than one reference */
    *vsetaddr = nvset;

    return 0;
}

static int vset__walk_copy(ti_thing_t * t, ti_vset_t * vset)
{
    ti_thing_t ** tarr = &t;

    ti_incref(t);

    /* deep is stored as flag */
    if (ti_thing_copy(tarr, vset->flags) || ti_vset_add(vset, *tarr))
    {
        ti_val_drop((ti_val_t *) *tarr);
        ti_vset_destroy(vset);
        return -1;
    }

    return 0;
}

static int vset__walk_dup(ti_thing_t * t, ti_vset_t * vset)
{
    ti_thing_t ** tarr = &t;

    ti_incref(t);

    /* deep is stored as flag */
    if (ti_thing_dup(tarr, vset->flags) || ti_vset_add(vset, *tarr))
    {
        ti_val_drop((ti_val_t *) *tarr);
        ti_vset_destroy(vset);
        return -1;
    }

    return 0;
}

int ti_vset_copy(ti_vset_t ** vsetaddr, uint8_t deep)
{
    assert (deep);

    ti_vset_t * nvset, * ovset = *vsetaddr;

    if (!(nvset = ti_vset_create()))
        return -1;

    /* set temporary `deep` as flag */
    nvset->flags = deep;

    if (imap_walk(ovset->imap, (imap_cb) vset__walk_copy, nvset))
        return -1;  /* vset is destroyed if walk has failed */

    /* set default flags */
    nvset->flags = 0;

    ti_val_unsafe_drop((ti_val_t *) ovset);
    *vsetaddr = nvset;

    return 0;
}

int ti_vset_dup(ti_vset_t ** vsetaddr, uint8_t deep)
{
    assert (deep);

    ti_vset_t * nvset, * ovset = *vsetaddr;

    if (!(nvset = ti_vset_create()))
        return -1;

    /* set temporary `deep` as flag */
    nvset->flags = deep;

    if (imap_walk(ovset->imap, (imap_cb) vset__walk_dup, nvset))
        return -1;  /* vset is destroyed if walk has failed */

    /* set default flags */
    nvset->flags = 0;

    ti_val_unsafe_drop((ti_val_t *) ovset);
    *vsetaddr = nvset;

    return 0;
}


/*
 * Increments the reference for each moved value to the set.
 * The return value is <0 in case of an error and `e` will contain the reason,
 * If 0, then the `thing` was already in the set, and 1 if it was added.
 */
int ti_vset_add_val(ti_vset_t * vset, ti_val_t * val, ex_t * e)
{
    uint16_t spec = ti_vset_spec(vset);

    if (!ti_val_is_thing(val))
    {
        ex_set(e, EX_TYPE_ERROR,
                "cannot add type `%s` to a "TI_VAL_SET_S,
                ti_val_str(val));
        return e->nr;
    }

    if (spec != TI_SPEC_ANY)
    {
        ti_thing_t * thing = (ti_thing_t *) val;
        ti_field_t * field = vset->key_;

        if (spec != thing->type_id)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "type `%s` is not allowed in restricted "TI_VAL_SET_S,
                    ti_val_str(val));
            return e->nr;
        }

        assert (vset->parent);

        if (!vset->parent->id && thing->id)
        {
            ex_set(e, EX_TYPE_ERROR,
                    "relations between stored and non-stored things must be "
                    "created using the property on the the stored thing "
                    "(the thing with an ID)");
            return e->nr;
        }

        if (field->condition.rel)
        {
            ti_field_t * ofield = field->condition.rel->field;
            if (ofield->spec == TI_SPEC_SET)
            {
                if (field != ofield || thing != vset->parent)
                    ofield->condition.rel->add_cb(ofield, thing, vset->parent);
            }
            else
            {
                ti_thing_t * othing = VEC_get(thing->items.vec, ofield->idx);
                if (othing->tp == TI_VAL_THING)
                    field->condition.rel->del_cb(field, othing, thing);

                ofield->condition.rel->add_cb(ofield, thing, vset->parent);
            }
        }
    }

    switch((imap_err_t) ti_vset_add(vset, (ti_thing_t *) val))
    {
    case IMAP_ERR_EXIST:
        return 0;
    case IMAP_ERR_ALLOC:
        ex_set_mem(e);
        ti_panic("unrecoverable error");
        return e->nr;
    case IMAP_SUCCESS:
        ti_incref(val);
    }
    return 1;
}

typedef struct
{
    ti_field_t * ofield;
    ti_thing_t * parent;
} vset__clear_t;

static int vset__clear_walk_cb(ti_thing_t * thing, vset__clear_t * w)
{
    w->ofield->condition.rel->del_cb(
            w->ofield,
            thing,
            w->parent);
    return 0;
}

void ti_vset_clear(ti_vset_t * vset)
{
    if (ti_vset_has_relation(vset))
    {
        ti_field_t * field = vset->key_;
        vset__clear_t w = {
                .ofield = field->condition.rel->field,
                .parent = vset->parent,
        };
        (void) imap_walk(vset->imap, (imap_cb) vset__clear_walk_cb, &w);
    }
    imap_clear(vset->imap, (imap_destroy_cb) ti_val_unsafe_gc_drop);
}

