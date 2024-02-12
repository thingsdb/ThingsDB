#ifndef TI_OPR_OR_H_
#define TI_OPR_OR_H_

#include <ti/opr/oprinc.h>

static int opr__or(ti_val_t * a, ti_val_t ** b, ex_t * e, _Bool inplace)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "bitwise `|` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        int_ = VINT(a) | VINT(*b);
        break;

    case OPR_INT_BOOL:
        int_ = VINT(a) | VBOOL(*b);
        break;

    case OPR_BOOL_INT:
        int_ = VBOOL(a) | VINT(*b);
        break;

    case OPR_BOOL_BOOL:
        int_ = VBOOL(a) | VBOOL(*b);
        break;

    case OPR_SET_SET:
    {
        ti_field_t * field = ti_vset_field((ti_vset_t *) a);
        _Bool unrestricted = !field || field->nested_spec == TI_SPEC_ANY;
        /*
         * The resulting set might contain items from both `a` and `b`,
         * therefore a "shortcut" can only be made if `b` is not used anymore
         * and this is an in-place modification of `a`, or `a` is also not
         * used anymore.
         */
        if (inplace && ti_val_test_unlocked(a, e))
            return e->nr;

        if (unrestricted && (inplace || a->ref == 1) && (*b)->ref == 1)
        {
            imap_union_move(VSET(a), VSET(*b));
            ti_val_unsafe_drop(*b);
            ti_incref(a);
            *b = a;
        }
        else
        {
            imap_t * imap = imap_create();
            if (!imap || imap_union_make(imap, VSET(a), VSET(*b)))
                goto alloc_err;

            if (inplace)
            {
                if (field && ti_field_vset_pre_assign(
                            field,
                            imap,
                            ((ti_vset_t *) a)->parent,
                            e,
                            true /* do type check */))
                {
                    imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
                    return e->nr;
                }
                ti_val_unsafe_drop(*b);
                imap_destroy(VSET(a), (imap_destroy_cb) ti_val_unsafe_gc_drop);
                VSET(a) = imap;
                ti_incref(a);
                *b = a;
                return e->nr;
            }
            ti_val_unsafe_drop(*b);
            *b = (ti_val_t *) ti_vset_create_imap(imap);
            if (!*b)
            {
                imap_destroy(imap, (imap_destroy_cb) ti_val_unsafe_drop);
                goto alloc_err;
            }
        }
        return e->nr;
    }
    }
    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;

alloc_err:
    ex_set_mem(e);
    return e->nr;
}

#endif /* TI_OPR_OR_H_ */
