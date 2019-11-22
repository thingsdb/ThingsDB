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

#include <ti/wrap.h>
#include <ti/regex.h>
#include <ti/varr.h>
#include <ti/verror.h>
#include <ti/vset.h>

int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t ** b, ex_t * e)
{
    switch (nd->len)
    {
    case 1:
        switch (*nd->str)
        {
        case '%':
            return opr__mod(a, b, e);
        case '&':
            return opr__and(a, b, e);
        case '*':
            return opr__mul(a, b, e);
        case '+':
            return opr__add(a, b, e);
        case '-':
            return opr__sub(a, b, e);
        case '/':
            return opr__div(a, b, e);
        case '<':
            return opr__lt(a, b, e);
        case '>':
            return opr__gt(a, b, e);
        case '^':
            return opr__xor(a, b, e);
        case '|':
            return opr__or(a, b, e);
        }
        break;
    case 2:
        switch (*nd->str)
        {
        case '!':
            assert (nd->str[1] == '=');
            return opr__ne(a, b, e);
        case '%':
            assert (nd->str[1] == '=');
            return opr__mod(a, b, e);
        case '&':
            assert (nd->str[1] == '=');
            return opr__and(a, b, e);
        case '*':
            assert (nd->str[1] == '=');
            return opr__mul(a, b, e);
        case '+':
            assert (nd->str[1] == '=');
            return opr__add(a, b, e);
        case '-':
            assert (nd->str[1] == '=');
            return opr__sub(a, b, e);
        case '/':
            assert (nd->str[1] == '=');
            return opr__div(a, b, e);
        case '<':
            assert (nd->str[1] == '=');
            return opr__le(a, b, e);
        case '=':
            assert (nd->str[1] == '=');
            return opr__eq(a, b, e);
        case '>':
            assert (nd->str[1] == '=');
            return opr__ge(a, b, e);
        case '^':
            assert (nd->str[1] == '=');
            return opr__xor(a, b, e);
        case '|':
            assert (nd->str[1] == '=');
            return opr__or(a, b, e);
        }
    }
    assert (0);
    return e->nr;
}

_Bool ti__opr_eq_(ti_val_t * a, ti_val_t * b)
{
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_NIL:
        return  a->tp == b->tp;
    case TI_VAL_INT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            return false;
        case TI_VAL_INT:
            return OPR__INT(a) == OPR__INT(b);
        case TI_VAL_FLOAT:
            return OPR__INT(a) == OPR__FLOAT(b);
        case TI_VAL_BOOL:
            return OPR__INT(a) == OPR__BOOL(b);
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            return false;
        case TI_VAL_INT:
            return OPR__FLOAT(a) == OPR__INT(b);
        case TI_VAL_FLOAT:
            return OPR__FLOAT(a) == OPR__FLOAT(b);
        case TI_VAL_BOOL:
            return OPR__FLOAT(a) == OPR__BOOL(b);
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            return false;
        case TI_VAL_INT:
            return  OPR__BOOL(a) == OPR__INT(b);
        case TI_VAL_FLOAT:
            return OPR__BOOL(a) == OPR__FLOAT(b);
        case TI_VAL_BOOL:
            return OPR__BOOL(a) == OPR__BOOL(b);
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            return false;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
            return ti_raw_eq((ti_raw_t *) a, (ti_raw_t *) b);
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_REGEX:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
            return false;
        case TI_VAL_REGEX:
            return ti_regex_eq((ti_regex_t *) a, (ti_regex_t *) b);
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_THING:
        return a == b;
    case TI_VAL_WRAP:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
            return false;
        case TI_VAL_WRAP:
            return  ((ti_wrap_t *) a)->type_id == ((ti_wrap_t *) b)->type_id &&
                    ((ti_wrap_t *) a)->thing == ((ti_wrap_t *) b)->thing;
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_ARR:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
            return false;
        case TI_VAL_ARR:
            return ti_varr_eq((ti_varr_t *) a, (ti_varr_t *) b);
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_SET:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
            return false;
        case TI_VAL_SET:
            return ti_vset_eq((ti_vset_t *) a, (ti_vset_t *) b);
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            return false;
        }
        break;
    case TI_VAL_CLOSURE:
        return false;
    case TI_VAL_ERROR:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
            return false;
        case TI_VAL_ERROR:
            return ((ti_verror_t *) a)->code == ((ti_verror_t *) b)->code;
        }
        break;
    }
    return false;
}

/*
 * Return `< 0` when `a < b`, `> 0` when `a > b` and 0 when equal.
 * If two types cannot be compared, `e` will be set.
 */
int ti_opr_compare(ti_val_t * a, ti_val_t * b, ex_t * e)
{
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_NIL:
        break;
    case TI_VAL_INT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            break;
        case TI_VAL_INT:
            return  (OPR__INT(a) > OPR__INT(b)) -
                    (OPR__INT(a) < OPR__INT(b));
        case TI_VAL_FLOAT:
            return  (OPR__INT(a) > OPR__FLOAT(b)) -
                    (OPR__INT(a) < OPR__FLOAT(b));
        case TI_VAL_BOOL:
            return  (OPR__INT(a) > OPR__BOOL(b)) -
                    (OPR__INT(a) < OPR__BOOL(b));
            break;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            break;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            break;
        case TI_VAL_INT:
            return  (OPR__FLOAT(a) > OPR__INT(b)) -
                    (OPR__FLOAT(a) < OPR__INT(b));
        case TI_VAL_FLOAT:
            return  (OPR__FLOAT(a) > OPR__FLOAT(b)) -
                    (OPR__FLOAT(a) < OPR__FLOAT(b));
        case TI_VAL_BOOL:
            return  (OPR__FLOAT(a) > OPR__BOOL(b)) -
                    (OPR__FLOAT(a) < OPR__BOOL(b));
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            break;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
            break;
        case TI_VAL_INT:
            return  (OPR__BOOL(a) > OPR__INT(b)) -
                    (OPR__BOOL(a) < OPR__INT(b));
        case TI_VAL_FLOAT:
            return  (OPR__BOOL(a) > OPR__FLOAT(b)) -
                    (OPR__BOOL(a) < OPR__FLOAT(b));
        case TI_VAL_BOOL:
            return  (OPR__BOOL(a) > OPR__BOOL(b)) -
                    (OPR__BOOL(a) < OPR__BOOL(b));
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            break;
        }
        break;
    case TI_VAL_MP:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            break;
        case TI_VAL_MP:
        case TI_VAL_NAME:
        case TI_VAL_STR:
        case TI_VAL_BYTES:
            return ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) b);
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_WRAP:
        case TI_VAL_ARR:
        case TI_VAL_SET:
        case TI_VAL_CLOSURE:
        case TI_VAL_ERROR:
            break;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
        break;
    }

    if (!e->nr)
        ex_set(e, EX_TYPE_ERROR, "`<` not supported between `%s` and `%s`",
            ti_val_str(a), ti_val_str(b));

    return 0;
}
