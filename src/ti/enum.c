/*
 * ti/enum.c
 */
#include <ti/enum.h>

ti_enum_t * ti_enum_create(
        ti_enums_t * enums,
        uint16_t enum_id,
        const char * name,
        size_t name_n,
        uint64_t created_at,
        uint64_t modified_at)
{
    ti_enum_t * enum_ = malloc(sizeof(ti_enum_t));
    if (!enum_)
        return NULL;

    enum_->refcount = 0;
    enum_->enum_id = enum_id;
    enum_->flags = 0;
    enum_->name = strndup(name, name_n);
    enum_->rname = ti_str_create(name, name_n);
    enum_->smap = smap_create();
    enum_->vec = NULL;
    enum_->created_at = created_at;
    enum_->modified_at = modified_at;

    if (!enum_->name || !enum_->rname || !enum_->smap || ti_enums_add(enums, enum_))
    {
        ti_enum_destroy(enum_);
        return NULL;
    }

    return enum_;
}

void ti_enum_drop(ti_enum_t * enum_)
{
    uintptr_t type_id;

    if (!enum_)
        return;


    ti_enum_del(type->types, type);
    ti_enum_destroy(type);
}

void ti_enum_destroy(ti_enum_t * enum_)
{
    if (!enum_)
        return;

    vec_destroy(enum_->vec, NULL);
    smap_destroy(enum_->smap, (vec_destroy_cb) ti_val_drop);
    ti_val_drop((ti_val_t *) type->rname);
    ti_val_drop((ti_val_t *) type->rwname);
    free(type->dependencies);
    free(type->name);
    free(type->wname);
    free(type);
}

//const char * ti_enum_tp_str(ti_enum_t * enum_)
//{
//    switch((ti_enum_enum) enum_->tp)
//    {
//    case TI_ENUM_INT:    return TI_VAL_INT_S;
//    case TI_ENUM_FLOAT:  return TI_VAL_FLOAT_S;
//    case TI_ENUM_STR:    return TI_VAL_STR_S;
//    case TI_ENUM_BYTES:  return TI_VAL_BYTES_S;
//    case TI_ENUM_THING:  return TI_VAL_THING_S;
//    }
//    assert(0);
//    return "unknown";
//}

static int enum__set_enum_tp(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_INT:            enum_->tp = TI_ENUM_INT;    break;
    case TI_VAL_FLOAT:          enum_->tp = TI_ENUM_FLOAT;  break;
    case TI_VAL_NAME:
    case TI_VAL_STR:            enum_->tp = TI_ENUM_STR;    break;
    case TI_VAL_BYTES:          enum_->tp = TI_ENUM_BYTES;  break;
    case TI_VAL_THING:
        enum_->tp = val->tp;
        break;
    default:
        ex_set(e, EX_TYPE_ERROR,
                "enum `%s` cannot be created; "
                "enumerators cannot be created for values of type `%s`",
                enum_->name,
                ti_val_str(val));
    }
    return e->nr;
}

static inline int enum__check_val(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    switch((ti_enum_enum) enum_->tp)
    {
    case TI_ENUM_INT:
        if (ti_val_is_int(val))
            return 0;
        break;
    case TI_ENUM_FLOAT:
        if (ti_val_is_float(val))
            return 0;
        break;
    case TI_ENUM_STR:
        if (ti_val_is_str(val))
            return 0;
        break;
    case TI_ENUM_BYTES:
        if (ti_val_is_bytes(val))
            return 0;
        break;
    case TI_ENUM_THING:
        if (ti_val_is_thing(val))
            return 0;
        break;
    }

    ex_set(e, EX_TYPE_ERROR,
            "enum `%s` cannot be created for mixed types; "
            "got both type `%s` and `%s`",
            enum_->name,
            ti_enum_tp_str(enum_),
            ti_val_str(val));

    return e->nr;
}

static int enum__init_thing_o(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    ti_prop_t * prop = vec_get(thing->items, 0);
    ti_venum_t * venum;

    if (enum__set_enum_tp(enum_, prop->val, e))
        return e->nr;

    enum_->tp = prop->val->tp;

    for (vec_each(thing->items, ti_prop_t, prop))
    {
        if (enum__check_val(enum_, prop->val, e))
            return e->nr;

        venum = ti_venum_create(enum_, prop->name, prop->val, enum_->vec->n);
        if (!venum)
        {
            ex_set_mem(e);
            /* TODO: revert?             */
            return e->nr;
        }




        VEC_push(enum_->vec, venum);
        if (!idx)
        {

        }
    }
    return 0;

}

static int enum__init_thing_t(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{

}



int ti_enum_init_from_thing(ti_enum_t * enum_, ti_thing_t * thing, ex_t * e)
{
    if (!thing->items->n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "enumerators must contain at least one value");
        return e->nr;
    }

    enum_->vec = vec_new(thing->items->n);
    if (!enum_->vec)
    {
        ex_set_mem(e);
        return e->nr;
    }

    return ti_thing_is_object(thing)
        ? enum__init_thing_o(enum_, thing, e)
        : enum__init_thing_t(enum_, thing, e);
}

ti_venum_t * ti_enum_val_by_val_e(ti_enum_t * enum_, ti_val_t * val, ex_t * e)
{
    for(vec_each(enum_->vec, ti_venum_t, venum))
        if (ti_opr_eq(VENUM(venum), val))
            return venum;
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_BOOL:
    case TI_VAL_MP:
    case TI_VAL_REGEX:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_CLOSURE:
    case TI_VAL_ERROR:
    case TI_VAL_ENUM:
    case TI_VAL_TEMPLATE:
        break;
    case TI_VAL_INT:
        if (enum_->tp != TI_ENUM_INT)
            goto type_err;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with value %"PRId64,
                enum_->name,
                VINT(val));
        return NULL;
    case TI_VAL_FLOAT:
        if (enum_->tp != TI_ENUM_INT)
            goto type_err;
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member with value %f",
                enum_->name,
                VFLOAT(val));
        return NULL;
    case TI_VAL_NAME:
    case TI_VAL_STR:            return TI_VAL_STR_S;
    case TI_VAL_BYTES:          return TI_VAL_BYTES_S;
    case TI_VAL_THING:          return ti_thing_is_object((ti_thing_t *) val)
                                    ? TI_VAL_THING_S
                                    : ti_thing_type_str((ti_thing_t *) val);
        assert (0);
    }



type_err:
    ex_set(e, EX_TYPE_ERROR,
            "enum `%s` is of type `%s` while the "
            "given value is of type `%s`",
            enum_->name,
            ti_enum_str_tp(enum_),
            ti_val_str(val));
    return NULL;
}
