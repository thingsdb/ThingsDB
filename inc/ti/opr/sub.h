#include <ti/opr/oprinc.h>

static int opr__sub(ti_val_t * a, ti_val_t ** b, ex_t * e, _Bool inplace)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0;    /* set to 0 only to prevent warning */
    ti_opr_perm_t perm = TI_OPR_PERM(a, *b);
    switch(perm)
    {
    default:
        ex_set(e, EX_TYPE_ERROR,
                "`-` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(*b));
        return e->nr;

    case OPR_INT_INT:
        if ((VINT(*b) < 0 && VINT(a) > LLONG_MAX + VINT(*b)) ||
            (VINT(*b) > 0 && VINT(a) < LLONG_MIN + VINT(*b)))
            goto overflow;
        int_ = VINT(a) - VINT(*b);
        goto type_int;

    case OPR_INT_FLOAT:
        float_ = VINT(a) - VFLOAT(*b);
        goto type_float;

    case OPR_INT_BOOL:
        if (VINT(a) == LLONG_MIN && VBOOL(*b))
            goto overflow;
        int_ = VINT(a) - VBOOL(*b);
        goto type_int;

    case OPR_FLOAT_INT:
        float_ = VFLOAT(a) - VINT(*b);
        goto type_float;

    case OPR_FLOAT_FLOAT:
        float_ = VFLOAT(a) - VFLOAT(*b);
        goto type_float;

    case  OPR_FLOAT_BOOL:
        float_ = VFLOAT(a) - VBOOL(*b);
        goto type_float;

    case OPR_BOOL_INT:
        if (VINT(*b) == LLONG_MIN ||
            (VBOOL(a) && VINT(*b) == -LLONG_MAX))
            goto overflow;
        int_ = VBOOL(a) - VINT(*b);
        goto type_int;

    case OPR_BOOL_FLOAT:
        float_ = VBOOL(a) - VFLOAT(*b);
        goto type_float;

    case OPR_BOOL_BOOL:
        int_ = VBOOL(a) - VBOOL(*b);
        goto type_int;

    case OPR_SET_SET:
    {
        ti_field_t * field = ti_vset_field((ti_vset_t *) a);
        _Bool unrestricted = !field || field->nested_spec == TI_SPEC_ANY;
        /*
         * The resulting set will only contain things which already exist
         * in `a`, therefore a "shortcut" can be made if this is an in-place
         * modification or if `a` is not used  anymore.
         */
        if (inplace && ti_val_test_unlocked(a, e))
            return e->nr;

        if (unrestricted && (inplace || a->ref == 1))
        {
            imap_difference_inplace(VSET(a), VSET(*b));
            ti_val_unsafe_drop(*b);
            ti_incref(a);
            *b = a;
        }
        else
        {
            imap_t * imap = imap_create();
            if (!imap || imap_difference_make(imap, VSET(a), VSET(*b)))
                goto alloc_err;

            if (inplace)
            {
                if (field && ti_field_vset_pre_assign(
                            field,
                            imap,
                            ((ti_vset_t *) a)->parent,
                            e,
                            false /* no type check */))
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
    assert (0);

type_float:
    if (ti_val_make_float(b, float_))
        ex_set_mem(e);
    return e->nr;

type_int:
    if (ti_val_make_int(b, int_))
        ex_set_mem(e);
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

alloc_err:
    ex_set_mem(e);
    return e->nr;
}
