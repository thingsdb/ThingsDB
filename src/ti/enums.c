/*
 * ti/enums.h
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/enum.h>
#include <ti/enums.h>
#include <ti/enums.inline.h>
#include <ti/types.h>
#include <ti/val.h>
#include <ti/val.inline.h>

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

int ti_enums_add(ti_enums_t * enums, ti_enum_t * enum_)
{
    if (imap_add(enums->imap, enum_->enum_id, enum_))
        return -1;

    if (smap_add(enums->smap, enum_->name, enum_))
    {
        (void) imap_pop(enums->imap, enum_->enum_id);
        return -1;
    }

    return 0;
}

int ti_enums_rename(ti_enums_t * enums, ti_enum_t * enum_, ti_raw_t * nname)
{
    char * name = strndup((const char *) nname->data, nname->n);
    if (!name || smap_add(enums->smap, name, enum_))
        return -1;

    (void) smap_pop(enums->smap, enum_->name);

    if (ti_types_rename_spec(
            enums->collection->types,
            enum_->enum_id | TI_ENUM_ID_FLAG,
            enum_->rname,
            nname))
    {
        ti_panic("failed to rename all specifications");
        return -1;
    }

    free(enum_->name);
    ti_val_unsafe_drop((ti_val_t *) enum_->rname);

    enum_->name = name;
    enum_->rname = nname;

    ti_incref(nname);
    return 0;
}

void ti_enums_del(ti_enums_t * enums, ti_enum_t * enum_)
{
    (void) imap_pop(enums->imap, enum_->enum_id);
    (void) smap_pop(enums->smap, enum_->name);
}

uint16_t ti_enums_get_new_id(ti_enums_t * enums, ex_t * e)
{
    uint16_t enum_id = imap_unused_id(enums->imap, TI_ENUM_ID_MASK);
    if (enum_id == TI_ENUM_ID_MASK)
    {
        ex_set(e, EX_MAX_QUOTA, "reached the maximum number of enumerators");
    }
    return enum_id;
}

static int enums__pack_enum(ti_enum_t * enum_, ti_varr_t * varr)
{
    ti_val_t * mpinfo = ti_enum_as_mpval(enum_);
    if (!mpinfo)
        return -1;
    VEC_push(varr->vec, mpinfo);
    return 0;
}

ti_varr_t * ti_enums_info(ti_enums_t * enums)
{
    ti_varr_t * varr = ti_varr_create(enums->imap->n);
    if (!varr)
        return NULL;

    if (smap_values(enums->smap, (smap_val_cb) enums__pack_enum, varr))
    {
        ti_val_unsafe_drop((ti_val_t *) varr);
        return NULL;
    }

    return varr;
}

int ti_enums_to_pk(ti_enums_t * enums, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, enums->imap->n) ||
        imap_walk(enums->imap, (imap_cb) ti_enum_to_pk, pk)
    );
}

