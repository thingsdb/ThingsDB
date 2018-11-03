/*
 * util/res.c
 */

#include <ti.h>
#include <ti/val.h>
#include <util/res.h>
#include <util/imap.h>

int res_set_unknown(ti_res_t * res, ti_thing_t * thing, ex_t * e)
{
    int rc = imap_add(res->collect, thing->id, thing);

    switch (rc)
    {
    case IMAP_SUCCESS:
        ti_incref(thing);
        break;
    case IMAP_ERR_ALLOC:
        ex_set_alloc(e);
        return e->nr;
    case IMAP_ERR_EXIST:
        break;
    }

    ti_val_clear(res->val);
    ti_val_weak_set(res->val, TI_VAL_UNKNOWN, NULL);

    return e->nr;
}
