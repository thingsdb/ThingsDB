/*
 * ti/opr.c
 */
#include <assert.h>
#include <ti/opr.h>

static int opr__gt(ti_val_t * a, ti_val_t * b, ex_t * e);

int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t * b, ex_t * e)
{
    switch (nd->len)
    {
    case 1:
        switch (*nd->str)
        {
        case '>':
            return opr__gt(a, b, e);

        }

    }
    assert (0);
    return e->nr;
}

static int opr__gt(ti_val_t * a, ti_val_t * b, ex_t * e)
{

    _Bool bool_ = false;
    switch ((ti_val_enum) a->tp)
    {
    case TI_VAL_PROP:
    case TI_VAL_UNDEFINED:
    case TI_VAL_NIL:
        goto type_err;
    case TI_VAL_INT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_PROP:
        case TI_VAL_UNDEFINED:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = a->via.int_ > b->via.int_;
            break;
        case TI_VAL_FLOAT:
            bool_ = a->via.int_ > b->via.float_;
            break;
        case TI_VAL_BOOL:
            bool_ = a->via.int_ > b->via.bool_;
            break;
        case TI_VAL_NAME:
        case TI_VAL_RAW:
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
        case TI_VAL_THING:
        case TI_VAL_THINGS:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_FLOAT:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_PROP:
        case TI_VAL_UNDEFINED:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = a->via.float_ > b->via.int_;
            break;
        case TI_VAL_FLOAT:
            bool_ = a->via.float_ > b->via.float_;
            break;
        case TI_VAL_BOOL:
            bool_ = a->via.float_ > b->via.bool_;
            break;
        case TI_VAL_NAME:
        case TI_VAL_RAW:
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
        case TI_VAL_THING:
        case TI_VAL_THINGS:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_BOOL:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_PROP:
        case TI_VAL_UNDEFINED:
        case TI_VAL_NIL:
            goto type_err;
        case TI_VAL_INT:
            bool_ = a->via.bool_ > b->via.int_;
            break;
        case TI_VAL_FLOAT:
            bool_ = a->via.bool_ > b->via.float_;
            break;
        case TI_VAL_BOOL:
            bool_ = a->via.bool_ > b->via.bool_;
            break;
        case TI_VAL_NAME:
        case TI_VAL_RAW:
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
        case TI_VAL_THING:
        case TI_VAL_THINGS:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_NAME:
        switch ((ti_val_enum) b->tp)
        {
        case TI_VAL_PROP:
        case TI_VAL_UNDEFINED:
        case TI_VAL_NIL:
        case TI_VAL_INT:
        case TI_VAL_FLOAT:
        case TI_VAL_BOOL:
            goto type_err;
        case TI_VAL_NAME:
            bool_ = strcmp(a->via.name->str, b->via.name->str) > 0;
            break;
        case TI_VAL_RAW:
        case TI_VAL_ARRAY:
        case TI_VAL_TUPLE:
        case TI_VAL_THING:
        case TI_VAL_THINGS:
        case TI_VAL_ARROW:
            goto type_err;
        }
        break;
    case TI_VAL_RAW:
        /* TODO: implement raw */
    case TI_VAL_ARRAY:
    case TI_VAL_TUPLE:
    case TI_VAL_THING:
    case TI_VAL_THINGS:
    case TI_VAL_ARROW:
        goto type_err;

    }

    ti_val_clear(b);
    ti_val_set_bool(b, bool_);

    return e->nr;

type_err:
    ex_set(e, EX_BAD_DATA, "`>` not supported between `%s` and `%s`",
        ti_val_str(a), ti_val_str(b));
    return e->nr;
}
