/*
 * ti/types.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/type.h>
#include <ti/types.h>
#include <ti/types.inline.h>
#include <ti/val.inline.h>


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

uint16_t ti_types_get_new_id(ti_types_t * types, ti_raw_t * rname, ex_t * e)
{
    uintptr_t utype;
    void * ptype;
    char name[TI_NAME_MAX+1];

    assert (rname->n <= TI_NAME_MAX);

    memcpy(name, rname->data, rname->n);
    name[rname->n] = '\0';

    ptype = smap_pop(types->removed, name);
    if (!ptype)
    {
        if (types->next_id == TI_SPEC_ANY)
        {
            ex_set(e, EX_MAX_QUOTA, "reached the maximum number of types");
        }
        return types->next_id;
    }

    utype = (uintptr_t) ptype;
    utype &= TI_TYPES_RM_MASK;

    assert (utype < types->next_id);
    assert (ti_types_by_id(types, utype) == NULL);

    return (uint16_t) utype;
}

typedef struct
{
    ti_varr_t * varr;
    _Bool with_definition;
} types__info_cb;

static int types__pack_type(ti_type_t * type, types__info_cb * w)
{
    ti_val_t * mpinfo = ti_type_as_mpval(type, w->with_definition);
    if (!mpinfo)
        return -1;
    VEC_push(w->varr->vec, mpinfo);
    return 0;
}

ti_varr_t * ti_types_info(ti_types_t * types, _Bool with_definition)
{
    ti_varr_t * varr = ti_varr_create(types->imap->n);
    if (!varr)
        return NULL;

    types__info_cb info_cb = {
            .varr = varr,
            .with_definition = with_definition,
    };

    if (imap_walk(types->imap, (imap_cb) types__pack_type, &info_cb))
    {
        ti_val_unsafe_drop((ti_val_t *) varr);
        return NULL;
    }

    return varr;
}

static inline int types__to_pk_cb(ti_type_t * type, msgpack_packer * pk)
{
    return ti_type_to_pk(type, pk, false);
}

int ti_types_to_pk(ti_types_t * types, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, types->imap->n) ||
        imap_walk(types->imap, (imap_cb) types__to_pk_cb, pk)
    );
}
