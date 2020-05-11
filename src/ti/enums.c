/*
 * ti/enums.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/type.h>
#include <ti/enum.h>
#include <ti/enums.h>
#include <ti/types.inline.h>


ti_enums_t * ti_enums_create(ti_collection_t * collection)
{
    ti_enums_t * enums = malloc(sizeof(ti_enums_t));
    if (!enums)
        return NULL;

    enums->imap = imap_create();
    enums->smap = smap_create();
    enums->collection = collection;

    if (!enums->imap || !enums->smap)
    {
        ti_enums_destroy(enums);
        return NULL;
    }

    return enums;
}

void ti_enums_destroy(ti_enums_t * enums)
{
    if (!enums)
        return;

    smap_destroy(enums->smap, NULL);
    imap_destroy(enums->imap, (imap_destroy_cb) ti_enum_destroy);
    free(enums);
}

uint16_t ti_enums_get_new_id(ti_enums_t * enums, ti_raw_t * rname, ex_t * e)
{
    uint16_t enum_id = imap_unused_id(enums->imap, TI_ENUM_ID_MASK);
    if (enum_id == TI_ENUM_ID_MASK)
    {
        ex_set(e, EX_MAX_QUOTA, "reached the maximum number of enumerators");
    }
    return enum_id;
}
