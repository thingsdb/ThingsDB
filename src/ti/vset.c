/*
 * ti/vset.c
 */
#include <assert.h>
#include <tiinc.h>
#include <stdlib.h>
#include <ti/vset.h>
#include <ti/val.h>
#include <util/logger.h>

ti_vset_t * ti_vset_create(void)
{
    ti_vset_t * vset = malloc(sizeof(ti_vset_t));
    if (!vset)
        return NULL;

    vset->ref = 1;
    vset->tp = TI_VAL_SET;

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

int ti_vset_to_packer(
        ti_vset_t * vset,
        qp_packer_t ** packer,
        int flags,
        int fetch)
{
    vec_t * vec = imap_vec(vset->imap);
    if (!vec ||
        qp_add_map(packer) ||
        qp_add_raw(*packer, (const uchar * ) "!", 1) ||
        qp_add_array(packer))
        return -1;
    fetch-=2;  /* one for the set and one for each thing */
    for (vec_each(vec, ti_thing_t, t))
    {
        if (((flags & TI_VAL_PACK_NEW) && ti_thing_is_new(t)) ||
            ((~flags & TI_VAL_PACK_NEW) && fetch > 0))
        {
            if (ti_thing_to_packer(t, packer, flags, fetch))
                return -1;
        }
        else
        {
            if (ti_thing_id_to_packer(t, packer))
                return -1;
        }
    }
    return qp_close_array(*packer) || qp_close_map(*packer);
}

int ti_vset_to_file(ti_vset_t * vset, FILE * f)
{
    vec_t * vec = imap_vec(vset->imap);
    if (    !vec ||
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar * ) "!", 1) ||
            qp_fadd_type(f, vec->n > 5 ? QP_ARRAY_OPEN: QP_ARRAY0 + vec->n))
        return -1;

    for (vec_each(vec, ti_thing_t, t))
        if (ti_thing_id_to_file(t, f))
            return -1;

    return vec->n > 5 ? qp_fadd_type(f, QP_ARRAY_CLOSE) : 0;
}

int ti_vset_assign(ti_vset_t ** vsetaddr)
{
    vec_t * vec;
    ti_vset_t * vset = *vsetaddr;

    if (vset->ref == 1)
        return 0;

    if (!(vec = imap_vec(vset->imap)) ||
        !(vset = ti_vset_create()))
        return -1;

    for (vec_each(vec, ti_thing_t, t))
    {
        if (ti_vset_add(vset, t))
        {
            ti_vset_destroy(vset);
            return -1;
        }
        ti_incref(t);
    }

    ti_decref(*vsetaddr);
    *vsetaddr = vset;

    return 0;
}

/*
 * Increments the reference for each moved value to the set.
 */
int ti_vset_add_val(ti_vset_t * vset, ti_val_t * val, ex_t * e)
{
    if (!ti_val_is_thing(val))
        ex_set(e, EX_BAD_DATA, "cannot add type `%s` to a "TI_VAL_SET_S,
                ti_val_str(val));
    else switch((imap_err_t) ti_vset_add(vset, (ti_thing_t *) val))
    {
    case IMAP_ERR_EXIST:
        break;
    case IMAP_ERR_ALLOC:
        ex_set_alloc(e);
        break;
    case IMAP_SUCCESS:
        ti_incref(val);
        break;
    }
    return e->nr;
}
