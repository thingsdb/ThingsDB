#include <ti/opr/oprinc.h>

static int opr__and(ti_val_t * a, ti_val_t ** b, ex_t * e, _Bool inplace)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "bitwise `&` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        int_ = VINT(a) & VINT(*b);
        break;

    case OPR_INT_BOOL:
        int_ = VINT(a) & VBOOL(*b);
        break;

    case OPR_BOOL_INT:
        int_ = VBOOL(a) & VINT(*b);
        break;

    case OPR_BOOL_BOOL:
        int_ = VBOOL(a) & VBOOL(*b);
        break;

    case OPR_SET_SET:
        /*
         * The resulting set will only contain thing which are already
         * available in both `a` and `b`, therefore a "shortcut" can be made
         * if this is an in-place modification or if `a` is not used  anymore.
         */
        if (ti_val_test_unlocked(a, e))
            return e->nr;

        if (ti_vset_is_unrestricted((ti_vset_t *) a) &&
            (inplace || a->ref == 1))
        {
            imap_intersection_inplace(
                    VSET(a),
                    VSET(*b),
                    (imap_destroy_cb) ti_val_unsafe_gc_drop);
            ti_val_unsafe_drop(*b);
            ti_incref(a);
            *b = a;
        }
        else
        {
            ti_vset_t * vset = ti_vset_create();
            if (!vset || imap_intersection_make(vset->imap, VSET(a), VSET(*b)))
                goto alloc_err;
            ti_val_unsafe_drop(*b);
            *b = (ti_val_t *) vset;
            if (inplace)
            {
                vset->parent = ((ti_vset_t *) a)->parent;
                vset->key_ = ((ti_vset_t *) a)->key_;
            }
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
