/*
 * ti/opr.c
 */
#include <ti/opr/add.h>
#include <ti/opr/and.h>
#include <ti/opr/div.h>
#include <ti/opr/eq.h>
#include <ti/opr/ge.h>
#include <ti/opr/gt.h>
#include <ti/opr/le.h>
#include <ti/opr/lt.h>
#include <ti/opr/mod.h>
#include <ti/opr/mul.h>
#include <ti/opr/ne.h>
#include <ti/opr/or.h>
#include <ti/opr/sub.h>
#include <ti/opr/xor.h>

#include <ti/member.h>
#include <ti/regex.h>
#include <ti/varr.h>
#include <ti/verror.h>
#include <ti/vset.h>
#include <ti/wrap.h>


int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t ** b, ex_t * e)
{
    switch (*nd->str)
    {
    case '!':
        return opr__ne(a, b, e);
    case '%':
        return opr__mod(a, b, e);
    case '&':
        return opr__and(a, b, e, nd->len == 2);
    case '*':
        return opr__mul(a, b, e);
    case '+':
        return opr__add(a, b, e);
    case '-':
        return opr__sub(a, b, e, nd->len == 2);
    case '/':
        return opr__div(a, b, e);
    case '<':
        return nd->len == 1 ? opr__lt(a, b, e) : opr__le(a, b, e);
    case '=':
        return opr__eq(a, b, e);
    case '>':
        return nd->len == 1 ? opr__gt(a, b, e) : opr__ge(a, b, e);
    case '^':
        return opr__xor(a, b, e, nd->len == 2);
    case '|':
        return opr__or(a, b, e, nd->len == 2);
    }
    assert (0);
    return e->nr;
}

_Bool ti__opr_eq_(ti_val_t * a, ti_val_t * b)
{
    ti_opr_perm_t perm = TI_OPR_PERM(a, b);
    switch (perm)
    {
    default:
        assert (a != b);
        return false;
    /*
    case OPR_NIL_NIL:
        no need to compare the nil values since they always
        point to the same address
    */
    case OPR_INT_INT:
        return VINT(a) == VINT(b);
    case OPR_INT_FLOAT:
        return VINT(a) == VFLOAT(b);
    case OPR_INT_BOOL:
        return VINT(a) == VBOOL(b);
    case OPR_FLOAT_INT:
        return VFLOAT(a) == VINT(b);
    case OPR_FLOAT_FLOAT:
        return VFLOAT(a) == VFLOAT(b);
    case OPR_FLOAT_BOOL:
        return VFLOAT(a) == VBOOL(b);
    case OPR_BOOL_INT:
        return  VBOOL(a) == VINT(b);
    case OPR_BOOL_FLOAT:
        return VBOOL(a) == VFLOAT(b);
    /*
    case OPR_BOOL_BOOL:
        boolean values true and false always point to the
        same address so they must be different since a != b
    */
    case OPR_MP_MP:
    case OPR_MP_NAME:
    case OPR_MP_STR:
    case OPR_MP_BYTES:
    case OPR_NAME_MP:
    /*
    case OPR_NAME_NAME:
        the same name values only exist once and must point to the
        same address so they must be different since a != b
    */
    case OPR_NAME_STR:
    case OPR_NAME_BYTES:
    case OPR_STR_MP:
    case OPR_STR_NAME:
    case OPR_STR_STR:
    case OPR_STR_BYTES:
    case OPR_BYTES_MP:
    case OPR_BYTES_NAME:
    case OPR_BYTES_STR:
    case OPR_BYTES_BYTES:
        return ti_raw_eq((ti_raw_t *) a, (ti_raw_t *) b);
    case OPR_REGEX_REGEX:
        return ti_regex_eq((ti_regex_t *) a, (ti_regex_t *) b);
    case OPR_WRAP_WRAP:
        return  ((ti_wrap_t *) a)->type_id == ((ti_wrap_t *) b)->type_id &&
                ((ti_wrap_t *) a)->thing == ((ti_wrap_t *) b)->thing;
    case OPR_ARR_ARR:
        return ti_varr_eq((ti_varr_t *) a, (ti_varr_t *) b);
    case OPR_SET_SET:
        return ti_vset_eq((ti_vset_t *) a, (ti_vset_t *) b);
    case OPR_ERROR_ERROR:
        return ((ti_verror_t *) a)->code == ((ti_verror_t *) b)->code;
    }
    return false;
}

/*
 * Return `< 0` when `a < b`, `> 0` when `a > b` and 0 when equal.
 * If two types cannot be compared, `e` will be set.
 */
int ti_opr_compare(ti_val_t * a, ti_val_t * b, ex_t * e)
{
    ti_opr_perm_t perm = TI_OPR_PERM(a, b);
    switch (perm)
    {
    default:
        /* careful, `e` might already be set, in which case we should not touch
         * the error value
         */
        if (ti_val_is_member(b))
            return ti_opr_compare(a, VMEMBER(b), e);

        if (!e->nr)
            ex_set(e, EX_TYPE_ERROR,
                "`<` not supported between `%s` and `%s`",
                ti_val_str(a), ti_val_str(b));

        return 0;
    case OPR_INT_INT:
        return (VINT(a) > VINT(b)) - (VINT(a) < VINT(b));
    case OPR_INT_FLOAT:
        return (VINT(a) > VFLOAT(b)) - (VINT(a) < VFLOAT(b));
    case OPR_INT_BOOL:
        return (VINT(a) > VBOOL(b)) - (VINT(a) < VBOOL(b));
    case OPR_FLOAT_INT:
        return (VFLOAT(a) > VINT(b)) - (VFLOAT(a) < VINT(b));
    case OPR_FLOAT_FLOAT:
        return (VFLOAT(a) > VFLOAT(b)) - (VFLOAT(a) < VFLOAT(b));
    case OPR_FLOAT_BOOL:
        return (VFLOAT(a) > VBOOL(b)) - (VFLOAT(a) < VBOOL(b));
    case OPR_BOOL_INT:
        return (VBOOL(a) > VINT(b)) - (VBOOL(a) < VINT(b));
    case OPR_BOOL_FLOAT:
        return (VBOOL(a) > VFLOAT(b)) - (VBOOL(a) < VFLOAT(b));
    case OPR_BOOL_BOOL:
        return (VBOOL(a) > VBOOL(b)) - (VBOOL(a) < VBOOL(b));
    case OPR_MP_MP:
    case OPR_MP_NAME:
    case OPR_MP_STR:
    case OPR_MP_BYTES:
    case OPR_NAME_MP:
    case OPR_NAME_NAME:
    case OPR_NAME_STR:
    case OPR_NAME_BYTES:
    case OPR_STR_MP:
    case OPR_STR_NAME:
    case OPR_STR_STR:
    case OPR_STR_BYTES:
    case OPR_BYTES_MP:
    case OPR_BYTES_NAME:
    case OPR_BYTES_STR:
    case OPR_BYTES_BYTES:
        return ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) b);
    case OPR_ENUM_NIL ... OPR_ENUM_ERROR:
        return ti_opr_compare(VMEMBER(a), b, e);
    case OPR_ENUM_ENUM:
        return ti_opr_compare(VMEMBER(a), VMEMBER(b), e);
    }
    return 0;
}
