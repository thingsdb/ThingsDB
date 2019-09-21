/*
 * ti/types.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/type.h>
#include <ti/types.h>


ti_types_t * ti_types_create(void)
{
    ti_types_t * types = malloc(sizeof(ti_types_t));
    if (!types)
        return NULL;

    types->imap = imap_create();
    types->smap = smap_create();

    if (!types->imap || !types->smap)
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
    imap_destroy(types->imap, (imap_destroy_cb) ti_type_destroy);
    free(types);
}

int ti_types_add(ti_types_t * types, ti_type_t * type)
{
    if (imap_add(types->imap, type->class, type))
        return -1;

    if (smap_add(types->smap, type->name, type))
    {
        (void) imap_pop(types->imap, type->class);
        return -1;
    }

    return 0;
}

/* Call ti_collection_del_type(..) so existing things using this type will
 * be converted to objects.
 */
void ti_types_del(ti_types_t * types, ti_type_t * type)
{
    assert (!type->refcount);
    (void) imap_pop(types->imap, type->class);
    (void) smap_pop(types->smap, type->name);
}

uint16_t ti_types_get_new_id(ti_types_t * types, ex_t * e)
{
    /* UINT16_MAX is reserved as TI_OBJECT_CLASS */
    uint16_t id = imap_unused_id(types->imap, UINT16_MAX);
    if (id == UINT16_MAX)
    {
        ex_set(e, EX_MAX_QUOTA, "reached the maximum number of types");
    }
    return id;
}
