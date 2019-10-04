/*
 * ti/types.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/type.h>
#include <ti/types.h>


ti_types_t * ti_types_create(ti_collection_t * collection)
{
    ti_types_t * types = malloc(sizeof(ti_types_t));
    if (!types)
        return NULL;

    types->imap = imap_create();
    types->smap = smap_create();
    types->removed = smap_create();
    types->collection = collection;
    types->next_id = 0;

    if (!types->imap || !types->smap || !types->removed)
    {
        ti_types_destroy(types);
        return NULL;
    }

    return types;
}

void ti_types_destroy(ti_types_t * types)
{
    if (!types)
        return;

    smap_destroy(types->smap, NULL);
    smap_destroy(types->removed, NULL);
    imap_destroy(types->imap, (imap_destroy_cb) ti_type_destroy);
    free(types);
}

int ti_types_add(ti_types_t * types, ti_type_t * type)
{
    if (imap_add(types->imap, type->type_id, type))
        return -1;

    if (smap_add(types->smap, type->name, type))
    {
        (void) imap_pop(types->imap, type->type_id);
        return -1;
    }

    if (type->type_id >= types->next_id)
        types->next_id = type->type_id + 1;

    return 0;
}

/*
 * Do not use this function directly; Call ti_type_del(..) so
 * existing things using this type will be converted to objects.
 */
void ti_types_del(ti_types_t * types, ti_type_t * type)
{
    assert (!type->refcount);
    (void) imap_pop(types->imap, type->type_id);
    (void) smap_pop(types->smap, type->name);
}

uint16_t ti_types_get_new_id(ti_types_t * types, ex_t * e)
{
    /* TI_SPEC_NILLABLE (bit 15) and TI_SPEC_ANY (bit 14) are used*/
    uint16_t id = imap_unused_id(types->imap, TI_SPEC_ANY);
    if (id == TI_SPEC_ANY)
    {
        ex_set(e, EX_MAX_QUOTA, "reached the maximum number of types");
    }
    return id;
}

static int types__approx_sz(ti_type_t * type, size_t * approx_sz)
{
    *approx_sz += ti_type_approx_pack_sz(type) - 27;
    return 0;
}

static int types__pack_sz(ti_type_t * type, qp_packer_t ** packer)
{
    return (
         qp_add_raw(*packer, (const uchar *) type->name, type->name_n) ||
         ti_type_fields_to_packer(type, packer)
     );
}

ti_val_t * ti_types_info_as_qpval(ti_types_t * types)
{
    ti_raw_t * rtypes = NULL;
    size_t approx_sz = 0;
    qp_packer_t * packer;

    (void) imap_walk(types->imap, (imap_cb) types__approx_sz, &approx_sz);

    packer = qp_packer_create2(approx_sz, 2);
    if (!packer)
        return NULL;

    if (qp_add_map(&packer) ||
        imap_walk(types->imap, (imap_cb) types__pack_sz, &packer) ||
        qp_close_map(packer))
        goto fail;

    rtypes = ti_raw_from_packer(packer);

fail:
    qp_packer_destroy(packer);
    return (ti_val_t *) rtypes;
}
