#include <ti/opr/oprinc.h>

static int opr__xor(ti_val_t * a, ti_val_t ** b, ex_t * e, _Bool inplace)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "bitwise `^` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        int_ = VINT(a) ^ VINT(*b);
        break;

    case OPR_INT_BOOL:
        int_ = VINT(a) ^  VBOOL(*b);
        break;

    case OPR_BOOL_INT:
        int_ = VBOOL(a) ^ VINT(*b);
        break;

    case OPR_BOOL_BOOL:
        int_ = VBOOL(a) ^ VBOOL(*b);
        break;

    case OPR_SET_SET:
        if ((inplace || a->ref == 1) && (*b)->ref == 1)
        {
            imap_symmdiff_move(
                    ((ti_vset_t *) a)->imap,
                    ((ti_vset_t *) *b)->imap,
                    (imap_destroy_cb) ti_val_drop);
            ti_val_drop(*b);
            ti_incref(a);
            *b = a;
        }
        else
        {
            ti_vset_t * vset = ti_vset_create();
            if (!vset || imap_symmdiff_make(
                    vset->imap,
                    ((ti_vset_t *) a)->imap,
                    ((ti_vset_t *) *b)->imap))
                goto alloc_err;
            ti_val_drop(*b);
            *b = (ti_val_t *) vset;
        }
        return e->nr;
    }
    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;

alloc_err:
    ex_set_mem(e);
    return e->nr;
}
