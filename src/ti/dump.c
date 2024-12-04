#include <ti/dump.h>
#include <ti/enum.h>
#include <ti/enums.h>
#include <ti/field.h>
#include <ti/procedure.h>
#include <ti/thing.h>
#include <ti/type.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <tiinc.h>
#include <util/buf.h>
#include <util/vec.h>
#include <util/mpack.h>

#define dump__key(pk__, comment__) \
    mp_pack_strn(pk__, "", 0)

static int dump__mark_new(ti_thing_t * thing, void * UNUSED(arg))
{
    ti_thing_mark_new(thing);
    return 0;
}

static int dump__unmark_new(ti_thing_t * thing, void * UNUSED(arg))
{
    ti_thing_unmark_new(thing);
    return 0;
}

static int dump__new_enum_cb(ti_enum_t * enum_, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_NEW_ENUM) ||
        msgpack_pack_map(pk, 4) ||

        dump__key(pk, "enum_id") ||
        msgpack_pack_uint16(pk, enum_->enum_id) ||

        dump__key(pk, "created_at") ||
        msgpack_pack_uint64(pk, enum_->created_at) ||

        dump__key(pk, "name") ||
        mp_pack_strn(pk, enum_->rname->data, enum_->rname->n) ||

        dump__key(pk, "size") ||
        msgpack_pack_uint32(pk, enum_->members->n)
    );
}

static int dump__new_type_cb(ti_type_t * type, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_NEW_TYPE) ||
        msgpack_pack_map(pk, 5) ||

        dump__key(pk, "type_id") ||
        msgpack_pack_uint16(pk, type->type_id) ||

        dump__key(pk, "created_at") ||
        msgpack_pack_uint64(pk, type->created_at) ||

        dump__key(pk, "name") ||
        mp_pack_strn(pk, type->rname->data, type->rname->n) ||

        dump__key(pk, "wrap_only") ||
        mp_pack_bool(pk, ti_type_is_wrap_only(type)) ||

        dump__key(pk, "hide_id") ||
        mp_pack_bool(pk, ti_type_hide_id(type))
    );
}

static int dump__set_type_cb(ti_type_t * type, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_SET_TYPE) ||
        msgpack_pack_map(pk, 6) ||

        dump__key(pk, "type_id") ||
        msgpack_pack_uint16(pk, type->type_id) ||

        dump__key(pk, "modified_at") ||
        msgpack_pack_uint64(pk, type->modified_at) ||

        dump__key(pk, "wrap_only") ||
        mp_pack_bool(pk, ti_type_is_wrap_only(type)) ||

        dump__key(pk, "hide_id") ||
        mp_pack_bool(pk, ti_type_hide_id(type)) ||

        dump__key(pk, "fields") ||
        ti_type_fields_to_pk(type, pk) ||

        dump__key(pk, "methods") ||
        ti_type_methods_to_pk(type, pk)
    );
}

static int dump__relation(ti_field_t * a, ti_field_t * b, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_MOD_TYPE_REL_ADD) ||
        msgpack_pack_map(pk, 5) ||

        dump__key(pk, "modified_at") ||
        msgpack_pack_uint64(pk, a->type->modified_at) ||

        dump__key(pk, "type1_id") ||
        msgpack_pack_uint16(pk, a->type->type_id) ||

        dump__key(pk, "name1") ||
        mp_pack_strn(pk, a->name->str, a->name->n) ||

        dump__key(pk, "type2_id") ||
        msgpack_pack_uint16(pk, b->type->type_id) ||

        dump__key(pk, "name2") ||
        mp_pack_strn(pk, b->name->str, b->name->n)
    );
}

static int dump__relation_cb(ti_type_t * type, msgpack_packer * pk)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * other = field->condition.rel->field;
            if ((   field == other ||
                    field->type->type_id < other->type->type_id ||
                    (   field->type->type_id == other->type->type_id &&
                        field->idx < other->idx)) &&
                    dump__relation(field, other, pk))
                return -1;
        }
    }
    return 0;
}

static int dump__rel_count_cb(ti_type_t * type, size_t * n)
{
    for (vec_each(type->fields, ti_field_t, field))
    {
        if (ti_field_has_relation(field))
        {
            ti_field_t * other = field->condition.rel->field;
            if ((   field == other ||
                    field->type->type_id < other->type->type_id ||
                    (   field->type->type_id == other->type->type_id &&
                        field->idx < other->idx)))
                *n += 1;
        }
    }
    return 0;
}

static int dump__procedure_cb(ti_procedure_t * procedure, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_NEW_PROCEDURE) ||
        msgpack_pack_map(pk, 3) ||

        dump__key(pk, "name") ||
        mp_pack_strn(pk, procedure->name, procedure->name_n) ||

        dump__key(pk, "created_at") ||
        msgpack_pack_uint64(pk, procedure->created_at) ||

        dump__key(pk, "closure") ||
        ti_closure_to_store_pk(procedure->closure, pk)
    );
}

static int dump__named_rooms_cb(ti_room_t * room,  msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_ROOM_SET_NAME) ||
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint64(pk, room->id) ||
        mp_pack_strn(pk, room->name->str, room->name->n)
    );
}

static int dump__enum_data_cb(ti_enum_t * enum_, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_SET_ENUM_DATA) ||
        msgpack_pack_map(pk, 3) ||

        dump__key(pk, "enum_id") ||
        msgpack_pack_uint16(pk, enum_->enum_id) ||

        dump__key(pk, "members") ||
        ti_enum_members_to_store_pk(enum_, pk) ||

        dump__key(pk, "methods") ||
        ti_enum_methods_to_store_pk(enum_, pk)
    );
}

static int dump__write_new_enums(ti_enums_t * enums, msgpack_packer * pk)
{
    return smap_values(enums->smap, (smap_val_cb) dump__new_enum_cb, pk);
}

static int dump__write_types(ti_types_t * types, msgpack_packer * pk)
{
    return (
            smap_values(types->smap, (smap_val_cb) dump__new_type_cb, pk) ||
            smap_values(types->smap, (smap_val_cb) dump__set_type_cb, pk)
    );
}

static int dump__write_tasks(vec_t * vtasks, msgpack_packer * pk)
{
    for (vec_each(vtasks, ti_vtask_t, vtask))
    {
        if (
            msgpack_pack_array(pk, 2) ||

            msgpack_pack_uint8(pk, TI_TASK_VTASK_NEW) ||
            msgpack_pack_map(pk, 5) ||

            dump__key(pk, "id") ||
            msgpack_pack_uint64(pk, vtask->id) ||

            dump__key(pk, "run_at") ||
            msgpack_pack_uint64(pk, vtask->run_at) ||

            dump__key(pk, "user_id") ||
            msgpack_pack_uint64(pk, 0) ||

            dump__key(pk, "closure") ||
            ti_closure_to_store_pk(vtask->closure, pk) ||

            dump__key(pk, "args") ||
            msgpack_pack_array(pk, 0)
        ) return -1;
    }
    return 0;
}

/*
 * Must write root before anything else as the replace root task removes
 * the existing root which otherwise might conflict with existing Id's.
 */
static int dump__write_root(ti_thing_t * root, msgpack_packer * pk)
{
    return (
        msgpack_pack_array(pk, 2) ||

        msgpack_pack_uint8(pk, TI_TASK_REPLACE_ROOT) ||
        ti_val_to_store_pk((ti_val_t *) root, pk)
    );
}

static int dump__write_enums_data(ti_enums_t * enums, msgpack_packer * pk)
{
    return smap_values(enums->smap, (smap_val_cb) dump__enum_data_cb, pk);
}

static int dump__write_tasks_args(vec_t * vtasks, msgpack_packer * pk)
{
    for (vec_each(vtasks, ti_vtask_t, vtask))
    {
        if (vtask->args->n)
        {
            if (
                msgpack_pack_array(pk, 2) ||

                msgpack_pack_uint8(pk, TI_TASK_VTASK_SET_ARGS) ||
                msgpack_pack_map(pk, 2) ||

                dump__key(pk, "id") ||
                msgpack_pack_uint64(pk, vtask->id) ||

                dump__key(pk, "args") ||
                msgpack_pack_array(pk, vtask->args->n)
            ) return -1;

            /*
             * In the @thingsdb scope there are no things allowed as arguments so
             * the code below should run fine
             */
            for (vec_each(vtask->args, ti_val_t, val))
                if (ti_val_to_store_pk(val, pk))
                    return -1;
        }
    }
    return 0;

}

static int dump__write_relations(ti_types_t * types, msgpack_packer * pk)
{
    /* It is very important that relations are being restored after all the
     * values are imported as otherwise relations will be created while
     * loading the values;
     */
    return smap_values(types->smap, (smap_val_cb) dump__relation_cb, pk);
}

static int dump__write_procedures(smap_t * procedures, msgpack_packer * pk)
{
    return smap_values(procedures, (smap_val_cb) dump__procedure_cb, pk);
}

static int dump__write_named_rooms(smap_t * named_rooms, msgpack_packer * pk)
{
    return smap_values(named_rooms, (smap_val_cb) dump__named_rooms_cb, pk);
}

static size_t dump__count_tasks(ti_collection_t * collection)
{
    size_t n = 1;  /* first is the root */
    n += collection->enums->smap->n * 2;
    n += collection->vtasks->n;
    for (vec_each(collection->vtasks, ti_vtask_t, vtask))
        if (vtask->args->n)
            n += 1;

    n += collection->types->smap->n * 2;
    smap_values(collection->types->smap, (smap_val_cb) dump__rel_count_cb, &n);
    n += collection->procedures->n;
    return n;
}


/*
 * Must not be called when things can be marked as new.
 * Check if no futures or change is used before calling this function.
 */
ti_raw_t * ti_dump_collection(ti_collection_t * collection)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_raw_t * raw;
    size_t n = dump__count_tasks(collection);

    if (mp_sbuffer_alloc_init(&buffer, 65536, sizeof(ti_raw_t)))
        return NULL;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2 + n);

    msgpack_pack_uint16(&pk, TI_EXPORT_SCHEMA_V1500);
    mp_pack_str(&pk, TI_VERSION);

    (void) imap_walk(collection->things, (imap_cb) dump__mark_new, NULL);

    if (dump__write_new_enums(collection->enums, &pk) ||
        dump__write_types(collection->types, &pk) ||
        dump__write_tasks(collection->vtasks, &pk) ||
        dump__write_root(collection->root, &pk) ||
        dump__write_enums_data(collection->enums, &pk) ||
        dump__write_tasks_args(collection->vtasks, &pk) ||
        dump__write_relations(collection->types, &pk) ||
        dump__write_procedures(collection->procedures, &pk) ||
        dump__write_named_rooms(collection->named_rooms, &pk))
    {
        /* failed, free buffer and remove marks */
        free(buffer.data);
        (void) imap_walk(collection->things, (imap_cb) dump__unmark_new, NULL);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_BYTES, buffer.size);
    return raw;
}

