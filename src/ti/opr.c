/*
 * ti/opr.c
 */
#include <assert.h>
#include <ti/opr.h>
#include <ti/raw.h>
#include <ti/vbool.h>
#include <ti/vint.h>
#include <ti/vfloat.h>
#include <util/logger.h>

#define CAST_MAX 9223372036854775808.0

static int opr__eq(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__ge(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__gt(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__le(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__lt(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__ne(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__add(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__sub(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__mul(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__div(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__idiv(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__mod(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__and(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__xor(ti_val_t * a, ti_val_t ** b, ex_t * e);
static int opr__or(ti_val_t * a, ti_val_t ** b, ex_t * e);


#define OPR__BOOL(__x)  ((ti_vbool_t *) __x)->bool_
#define OPR__INT(__x)   ((ti_vint_t *) __x)->int_
#define OPR__FLOAT(__x) ((ti_vfloat_t *) __x)->float_
#define OPR__RAW(__x)   ((ti_raw_t *) __x)

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
            return nd->str[1] == '=' && a->tp == TI_VAL_FLOAT
                ? opr__div(a, b, e)
                : opr__idiv(a, b, e);
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

static int opr__eq(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
        goto type_err;
    case TI_VAL_NIL:
        bool_ =  a->tp == (*b)->tp;
        break;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* false */
        case TI_VAL_INT:
            bool_ = OPR__INT(a) == OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) == OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) == OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* false */
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* false */
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) == OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) == OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) == OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* false */
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* false */
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) == OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) == OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) == OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* false */
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            break;  /* false */
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = ti_raw_equal((ti_raw_t *) a, (ti_raw_t *) *b);
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* false */
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        bool_ = a == *b;
        break;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`==` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__ge(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__INT(a) >= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) >= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) >= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) >= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) >= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) >= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) >= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) >= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) >= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) >= 0;
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`>=` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__gt(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__INT(a) > OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) > OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) > OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) > OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) > OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) > OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) > OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) > OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) > OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) > 0;
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`>` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__le(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__INT(a) <= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) <= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) <= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) <= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) <= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) <= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) <= OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) <= OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) <= OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) <= 0;
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`<=` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__lt(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__INT(a) < OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) < OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) < OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) < OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) < OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) < OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) < OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) < OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) < OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = ti_raw_cmp((ti_raw_t *) a, (ti_raw_t *) *b) < 0;
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`<` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__ne(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    _Bool bool_ = true;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
        goto type_err;
    case TI_VAL_NIL:
        bool_ =  a->tp != (*b)->tp;
        break;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* true */
        case TI_VAL_INT:
            bool_ = OPR__INT(a) != OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__INT(a) != OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__INT(a) != OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* true */
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* true */
        case TI_VAL_INT:
            bool_ = OPR__FLOAT(a) != OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__FLOAT(a) != OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__FLOAT(a) != OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* true */
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
            break;  /* true */
        case TI_VAL_INT:
            bool_ = OPR__BOOL(a) != OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            bool_ = OPR__BOOL(a) != OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            bool_ = OPR__BOOL(a) != OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* true */
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
            goto type_err;
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            break;  /* true */
        case TI_VAL_QP:
        case TI_VAL_RAW:
            bool_ = !ti_raw_equal((ti_raw_t *) a, (ti_raw_t *) *b);
            break;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            break;  /* true */
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        bool_ = a != *b;
        break;
    }

    ti_val_drop(*b);
    *b = (ti_val_t *) ti_vbool_get(bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`!=` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__add(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0f;   /* set to 0 only to prevent warning */
    ti_raw_t * raw = NULL;  /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if ((OPR__INT(*b) > 0 && OPR__INT(a) > LLONG_MAX - OPR__INT(*b)) ||
                (OPR__INT(*b) < 0 && OPR__INT(a) < LLONG_MIN - OPR__INT(*b)))
                goto overflow;
            int_ = OPR__INT(a) + OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__INT(a) + OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            if (OPR__INT(a) == LLONG_MAX && OPR__BOOL(*b))
                goto overflow;
            int_ = OPR__INT(a) + OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            float_ = OPR__FLOAT(a) + OPR__INT(*b);
            goto type_float;
        case TI_VAL_FLOAT:
            float_ = OPR__FLOAT(a) + OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            float_ = OPR__FLOAT(a) + OPR__BOOL(*b);
            goto type_float;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__BOOL(a) && OPR__INT(*b) == LLONG_MAX)
                goto overflow;
            int_ = OPR__BOOL(a) + OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__BOOL(a) + OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) + OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_QP:
        case TI_VAL_RAW:
            raw = ti_raw_cat((ti_raw_t *) a, (ti_raw_t *) *b);
            goto type_raw;
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    assert (0);

type_raw:
    if (raw)
    {
        ti_val_drop(*b);
        *b = (ti_val_t *) raw;
    }
    else
        ex_set_alloc(e);

    return e->nr;

type_float:

    if (ti_val_make_float(b, float_))
        ex_set_alloc(e);

    return e->nr;

type_int:

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`+` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

static int opr__sub(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0f;   /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if ((OPR__INT(*b) < 0 && OPR__INT(a) > LLONG_MAX + OPR__INT(*b)) ||
                (OPR__INT(*b) > 0 && OPR__INT(a) < LLONG_MIN + OPR__INT(*b)))
                goto overflow;
            int_ = OPR__INT(a) - OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__INT(a) - OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            if (OPR__INT(a) == LLONG_MIN && OPR__BOOL(*b))
                goto overflow;
            int_ = OPR__INT(a) - OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            float_ = OPR__FLOAT(a) - OPR__INT(*b);
            goto type_float;
        case TI_VAL_FLOAT:
            float_ = OPR__FLOAT(a) - OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            float_ = OPR__FLOAT(a) - OPR__BOOL(*b);
            goto type_float;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == LLONG_MIN ||
                (OPR__BOOL(a) && OPR__INT(*b) == -LLONG_MAX))
                goto overflow;
            int_ = OPR__BOOL(a) - OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__BOOL(a) - OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) - OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    assert (0);

type_float:

    if (ti_val_make_float(b, float_))
        ex_set_alloc(e);

    return e->nr;

type_int:

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`-` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

static int opr__mul(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */
    double float_ = 0.0f;   /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if ((OPR__INT(a) > LLONG_MAX / OPR__INT(*b)) ||
                (OPR__INT(a) < LLONG_MIN / OPR__INT(*b)) ||
                (OPR__INT(a) == -1 && OPR__INT(*b) == LLONG_MIN) ||
                (OPR__INT(*b) == -1 && OPR__INT(a) == LLONG_MIN))
                goto overflow;
            int_ = OPR__INT(a) * OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__INT(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) * OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            float_ = OPR__FLOAT(a) * OPR__INT(*b);
            goto type_float;
        case TI_VAL_FLOAT:
            float_ = OPR__FLOAT(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            float_ = OPR__FLOAT(a) * OPR__BOOL(*b);
            goto type_float;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) * OPR__INT(*b);
            goto type_int;
        case TI_VAL_FLOAT:
            float_ = OPR__BOOL(a) * OPR__FLOAT(*b);
            goto type_float;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) * OPR__BOOL(*b);
            goto type_int;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    assert (0);

type_float:

    if (ti_val_make_float(b, float_))
        ex_set_alloc(e);

    return e->nr;

type_int:

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`*` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;
}

static int opr__div(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    double float_ = 0.0f;   /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            float_ = (double) OPR__INT(a) / (double) OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = (double) OPR__INT(a) / OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            float_ = (double) OPR__INT(a) / (double) OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            float_ = OPR__FLOAT(a) / (double) OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = OPR__FLOAT(a) / OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            float_ = OPR__FLOAT(a) / (double) OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            float_ = (double) OPR__BOOL(a) / (double) OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            float_ = (double) OPR__BOOL(a) / OPR__FLOAT(*b);
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            float_ = (double) OPR__BOOL(a) / (double) OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_float(b, float_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`/` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}

static int opr__idiv(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;   /* set to 0 only to prevent warning */
    double d;

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            if (OPR__INT(a) == LLONG_MAX && OPR__INT(*b) == -1)
                goto overflow;
            int_ = OPR__INT(a) / OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            d = OPR__INT(a) / OPR__FLOAT(*b);
            if (ti_val_overflow_cast(d))
                goto overflow;
            int_ = (int64_t) d;
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            int_ = OPR__INT(a) / OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            d = OPR__FLOAT(a) / OPR__INT(*b);
            if (ti_val_overflow_cast(d))
                goto overflow;
            int_ = (int64_t) d;
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            d = OPR__FLOAT(a) / OPR__FLOAT(*b);
            if (ti_val_overflow_cast(d))
                goto overflow;
            int_ = (int64_t) d;
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            d = OPR__FLOAT(a) / OPR__BOOL(*b);
            if (ti_val_overflow_cast(d))
                goto overflow;
            int_ = (int64_t) d;
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            int_ = OPR__BOOL(a) / OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (OPR__FLOAT(*b) == 0.0)
                goto zerodiv;
            d = OPR__BOOL(a) / OPR__FLOAT(*b);
            if (ti_val_overflow_cast(d))
                goto overflow;
            int_ = (int64_t) d;
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            int_ = OPR__BOOL(a) / OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`//` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}

static int opr__mod(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            int_ = OPR__INT(a) % OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (ti_val_overflow_cast(OPR__FLOAT(*b)))
                goto overflow;
            int_ = (int64_t) OPR__FLOAT(*b);
            if (int_ == 0)
                goto zerodiv;
            int_ = OPR__INT(a) % int_;
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            int_ = OPR__INT(a) % OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (ti_val_overflow_cast(OPR__FLOAT(a)))
                goto overflow;
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            int_ = (int64_t) OPR__FLOAT(a) % OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (ti_val_overflow_cast(OPR__FLOAT(a)) ||
                ti_val_overflow_cast(OPR__FLOAT(*b)))
                goto overflow;
            int_ = (int64_t) OPR__FLOAT(*b);
            if (int_ == 0)
                goto zerodiv;
            int_ = (int64_t) OPR__FLOAT(a) % int_;
            break;
        case TI_VAL_BOOL:
            if (ti_val_overflow_cast(OPR__FLOAT(a)))
                goto overflow;
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            int_ = (int64_t) OPR__FLOAT(a) % OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            if (OPR__INT(*b) == 0)
                goto zerodiv;
            int_ = OPR__BOOL(a) % OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            if (ti_val_overflow_cast(OPR__FLOAT(*b)))
                goto overflow;
            int_ = (int64_t) OPR__FLOAT(*b);
            if (int_ == 0)
                goto zerodiv;
            int_ = OPR__BOOL(a) % int_;
            break;
        case TI_VAL_BOOL:
            if (OPR__BOOL(*b) == 0)
                goto zerodiv;
            int_ = OPR__BOOL(a) % OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`%` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;

overflow:
    ex_set(e, EX_OVERFLOW, "integer overflow");
    return e->nr;

zerodiv:
    ex_set(e, EX_ZERO_DIV, "division or modulo by zero");
    return e->nr;
}

static int opr__and(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__INT(a) & OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) & OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        goto type_err;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) & OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) & OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "bitwise `&` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__xor(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__INT(a) ^ OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) ^  OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        goto type_err;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) ^ OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) ^ OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "bitwise `^` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

static int opr__or(ti_val_t * a, ti_val_t ** b, ex_t * e)
{
    int64_t int_ = 0;       /* set to 0 only to prevent warning */

    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_ATTR:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__INT(a) | OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__INT(a) | OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        goto type_err;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) (*b)->tp)
        {
        case TI_VAL_ATTR:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            int_ = OPR__BOOL(a) | OPR__INT(*b);
            break;
        case TI_VAL_FLOAT:
            goto type_err;
        case TI_VAL_BOOL:
            int_ = OPR__BOOL(a) | OPR__BOOL(*b);
            break;
        case TI_VAL_QP:
        case TI_VAL_RAW:
        case TI_VAL_REGEX:
        case TI_VAL_THING:
        case TI_VAL_ARR:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_QP:
    case TI_VAL_RAW:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_ARR:
    case TI_VAL_ARROW:
        goto type_err;
    }

    if (ti_val_make_int(b, int_))
        ex_set_alloc(e);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "bitwise `|` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(*b));
    return e->nr;
}

