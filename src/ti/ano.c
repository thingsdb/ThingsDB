/*
 * ti/ano.c
 */
#include <ti/ano.h>
#include <ti/type.h>
#include <ti/raw.inline.h>
#include <ti/val.inline.h>
#include <util/logger.h>

ti_ano_t * ti_ano_new(void)
{
    ti_ano_t * ano = calloc(1, sizeof(ti_ano_t));
    if (!ano)
        return NULL;
    ano->ref = 1;
    ano->tp = TI_VAL_ANO;
    return ano;
}

int ano__dep_check(ti_type_t * type, ex_t * e)
{
    if (ti_type_has_dependencies(type))
        ex_set(e, EX_TYPE_ERROR,
            "the "TI_VAL_ANO_S" type definition "
            "contains dependencies on other types or enumerators, "
            "which is not permitted; to resolve this, create a fully "
            "named wrap-only type"DOC_SET_TYPE);
    return e->nr;
}

int ti_ano_init(
        ti_ano_t * ano,
        ti_collection_t * collection,
        ti_raw_t * spec_raw,
        ex_t * e)
{
    uint8_t flags = TI_TYPE_FLAG_WRAP_ONLY | TI_TYPE_FLAG_HIDE_ID;
    ti_type_t * type;
    ti_val_t * val;
    mp_unp_t up;
    ti_vup_t vup = {
            .isclient = true,
            .collection = collection,
            .up = &up,
    };
    mp_unp_init(&up, spec_raw->data, spec_raw->n);

    val = ti_val_from_vup_e(&vup, e);
    if (!val)
        return e->nr;

    if (!ti_val_is_object(val))
    {
        ex_set_internal(e);  /* only with corrupt internal data */
        goto fail0;
    }

    for (vec_each(((ti_thing_t *) val)->items.vec, ti_prop_t, prop))
    {
        ti_raw_t * sr = (ti_raw_t *) prop->val;
        if (ti_val_is_raw(prop->val) && sr->n)
        {
            if (sr->data[0] == '#')
                flags &= ~TI_TYPE_FLAG_HIDE_ID;

            if (sr->data[0] == '|')
            {
                ti_qbind_t syntax = {
                    .immutable_n = 0,
                    .flags = TI_QBIND_FLAG_COLLECTION,
                };
                ti_closure_t * closure = ti_closure_from_strn(
                        &syntax,
                        (const char *) sr->data,
                        sr->n,
                        e);
                if (!closure)
                    goto fail0;

                ti_val_unsafe_drop(prop->val);
                prop->val = (ti_val_t *) closure;
            }
        }
    }

    type = ti_type_create_anonymous(
            collection->types,
            (ti_raw_t *) ti_val_borrow_anonymous_name(),
            flags);
    if (!type)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (ti_type_init_from_thing(type, (ti_thing_t *) val, e) ||
        ano__dep_check(type, e))
        ti_type_drop_anonymous(type);
    else
    {
        ano->type = type;
        ano->spec_raw = spec_raw;
        ti_incref(spec_raw);
        (void) smap_addn(
            collection->ano_types,
            (const char *) spec_raw->data,
            spec_raw->n,
            ano);
    }
fail0:
    ti_val_unsafe_drop(val);
    return e->nr;
}

ti_ano_t * ti_ano_from_raw(
        ti_collection_t * collection,
        ti_raw_t * spec_raw,
        ex_t * e)
{
    ti_ano_t * ano = ti_ano_new();
    if (!ano)
    {
        ex_set_mem(e);
        return NULL;
    }
    if (ti_ano_init(ano, collection, spec_raw, e))
    {
        ti_ano_destroy(ano);
        return NULL;
    }
    /* This is used for quickly loading equal ano from cache, its a weak
     * ref and the call is not critical; It should be done here, and not
     * init as we want to cache ano(..) calls, and loading from changes/stored
     * on disk, but not from syntax in queries as they are cached as immutable
     * variable; */


    return ano;
}

ti_ano_t * ti_ano_create(
        ti_collection_t * collection,
        const unsigned char * bin,
        size_t n,
        ex_t * e)
{
    ti_raw_t * spec_raw;
    ti_ano_t * ano = smap_getn(collection->ano_types, (const char *) bin, n);
    if (ano)
    {
        ti_incref(ano);
        return ano;
    }
    spec_raw = ti_bin_create(bin, n);
    if (!spec_raw)
        return NULL;

    ano = ti_ano_from_raw(collection, spec_raw, e);
    ti_val_unsafe_drop((ti_val_t *) spec_raw);
    return ano;
}

void ti_ano_destroy(ti_ano_t * ano)
{
    /* we must check type as we might have spec_raw without a type */
    if (ano->type)
        (void) smap_popn(
            ano->type->types->collection->ano_types,
            (const char *) ano->spec_raw->data,
            ano->spec_raw->n);
    ti_val_drop((ti_val_t *) ano->spec_raw);
    ti_type_drop_anonymous(ano->type);
    free(ano);
}
