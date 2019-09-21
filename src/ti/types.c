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

uint16_t ti_types_get_new_id(ti_types_t * types, ex_t * e)
{
    uint16_t id = imap_unused_id(types->imap, UINT16_MAX);
    if (id == UINT16_MAX)
    {
        ex_set(e, EX_MAX_QUOTA, "reached the maximum number of types");
    }
    return id;
}
