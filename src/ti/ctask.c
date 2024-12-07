/*
 * ti/ctask.c (collection task)
 */
#include <assert.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/condition.h>
#include <ti/ctask.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/item.h>
#include <ti/member.inline.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/prop.h>
#include <ti/qbind.h>
#include <ti/raw.inline.h>
#include <ti/task.t.h>
#include <ti/thing.inline.h>
#include <ti/vtask.h>
#include <ti/vtask.inline.h>
#include <ti/types.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/verror.h>
#include <ti/vset.h>
#include <ti/vup.t.h>
#include <util/mpack.h>

/*
 * Returns 0 on success
 * - for example: {'prop': [new_count, values...]}
 */
static int ctask__set_add(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_val_t * val;
    ti_val_t * v;
    size_t i;
    mp_obj_t obj, mp_prop;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_prop) != MP_STR ||
        mp_next(up, &obj) != MP_ARR)
    {
        log_critical(
                "task `add` to set on "TI_THING_ID": "
                "missing map, property or new_count",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "task `add` to set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_set(val))
    {
        log_critical(
                "task `add` to set on "TI_THING_ID": "
                "expecting a `"TI_VAL_SET_S"`, got `%s`",
                thing->id,
                ti_val_str(val));
        return -1;
    }

    for (i = obj.via.sz; i--;)
    {
        v = ti_val_from_vup(&vup);
        if (!v || ti_vset_add_val((ti_vset_t *) val, v, &e) < 0)
        {
            log_critical(
                    "task `add` to set on "TI_THING_ID": "
                    "error while reading value for property",
                    thing->id);
            ti_val_drop(v);
            return -1;
        }
        ti_val_unsafe_drop(v);
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: ``
 */
static int ctask__thing_clear(ti_thing_t * thing, mp_unp_t * up)
{
    if (mp_skip(up) != MP_NIL)
    {
        log_critical(
                "task `clear` on "TI_THING_ID": expecting `nil`", thing->id);
        return -1;
    }

    if (ti_thing_is_dict(thing))
        smap_clear(
                thing->items.smap,
                (smap_destroy_cb) ti_item_unassign_destroy);
    else
        vec_clear_cb(
                thing->items.vec,
                (vec_destroy_cb) ti_prop_unassign_destroy);

    return 0;
}

static int ctask__room_set_name(ti_thing_t * thing, mp_unp_t * up)
{
    ti_room_t * room;
    mp_obj_t obj, mp_id, mp_name;

    if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(up, &mp_id) != MP_U64 ||
        (mp_next(up, &mp_name) != MP_STR && mp_name.tp != MP_NIL))
    {
        log_critical(
                "task `room_set_name` is "
                "expecting an array with integer and name/nil");
        return -1;
    }

    room = ti_collection_room_by_id(thing->collection, mp_id.via.u64);
    if (!room)
        return 0;  /* can happen when the room is gone */

    if (mp_name.tp == MP_NIL)
    {
        ti_room_unset_name(room);
    }
    else
    {
        ti_name_t * name = ti_names_get(
            mp_name.via.str.data,
            mp_name.via.str.n);
        if (!name || ti_room_set_name(room, name))
        {
            log_critical(EX_MEMORY_S);
            ti_name_drop(name);
            return -1;
        }
        ti_decref(name);
    }
    return 0;
}

/*
 * Returns 0 on success
 * - for example: `prop`
 */
static int ctask__arr_clear(ti_thing_t * thing, mp_unp_t * up)
{
    ti_val_t * val;
    mp_obj_t mp_prop;

    if (mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "task `clear` on array on "TI_THING_ID": "
                "expecting a property name",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "task `clear` on array on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_list(val))
    {
        log_critical(
                "task `clear` on array on "TI_THING_ID": "
                "expecting a `"TI_VAL_LIST_S"`, got `%s`",
                thing->id,
                ti_val_str(val));
        return -1;
    }

    vec_clear_cb(VARR(val), (vec_destroy_cb) ti_val_unsafe_gc_drop);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: `prop`
 */
static int ctask__set_clear(ti_thing_t * thing, mp_unp_t * up)
{
    ti_val_t * val;
    mp_obj_t mp_prop;

    if (mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "task `clear` on set on "TI_THING_ID": "
                "expecting a property name",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "task `clear` on set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_set(val))
    {
        log_critical(
                "task `clear` on set on "TI_THING_ID": "
                "expecting a `"TI_VAL_SET_S"`, got `%s`",
                thing->id,
                ti_val_str(val));
        return -1;
    }

    ti_vset_clear((ti_vset_t *) val);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'prop':value}
 */
static int ctask__set(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_val_t * val;
    ti_raw_t * key;
    mp_obj_t obj, mp_prop;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "task `set` to "TI_THING_ID": "
                "missing map or property",
                thing->id);
        return -1;
    }

    key = ti_name_is_valid_strn(mp_prop.via.str.data, mp_prop.via.str.n)
        ? (ti_raw_t *) ti_names_get(mp_prop.via.str.data, mp_prop.via.str.n)
        : ti_str_create(mp_prop.via.str.data, mp_prop.via.str.n);

    if (!key || ti_raw_is_reserved_key(key))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
    {
        log_critical(
                "task `set` to "TI_THING_ID": "
                "error reading value for property",
                thing->id);
        goto fail;
    }

    if (ti_thing_is_object(thing))
    {
        if (ti_val_make_assignable(&val, thing, key, &e))
        {
            log_critical(
                    "task `set` to "TI_THING_ID": "
                    "error making variable assignable: `%s`",
                    thing->id,
                    e.msg);
            goto fail;
        }

        if (ti_thing_o_set(thing, key, val))
        {
            log_critical(
                    "task `set` to "TI_THING_ID": "
                    "error setting property (type: `%s`)",
                    thing->id,
                    ti_val_str(val));
            goto fail;
        }
    }
    else
    {
        ti_field_t * field = \
                ti_field_by_name(thing->via.type, (ti_name_t *) key);
        if (!field)
        {
            log_critical(
                    "task `set` to "TI_THING_ID": "
                    "cannot find field",
                    thing->id);
            goto fail;
        }
        /* we no longer need the key, bug #291 (see extra comment issue) */
        ti_val_unsafe_drop((ti_val_t *) key);

        if (ti_field_make_assignable(field, &val, thing, &e))
        {
            log_critical(
                    "task `set` to "TI_THING_ID": "
                    "cannot make value relation",
                    thing->id);
            goto fail;
        }

        ti_thing_t_prop_set(thing, field, val);
    }

    return 0;

fail:
    ti_val_drop(val);
    ti_val_unsafe_drop((ti_val_t *) key);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'type_id':.., 'name':.. }
 *
 * Note: decided to `panic` in case of failures since it might mess up
 *       the database in case of failure.
 */
static int ctask__new_type(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_name, mp_created;
    uint8_t flags = 0;

    /*
     * TODO: (COMPAT) Before v0.9.7 the wrap_only value was not included.
     *       Some code can be simplified once backwards dependency may
     *       be dropped.
     */
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz < 3 || obj.via.sz > 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
            "task `new_type` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    if (obj.via.sz >= 4)
    {
        mp_obj_t mp_wpo;

        if (mp_skip(up) != MP_STR || mp_next(up, &mp_wpo) != MP_BOOL)
        {
            log_critical(
                "task `new_type` for "TI_COLLECTION_ID" is invalid",
                collection->id);
            return -1;
        }

        if (mp_wpo.via.bool_)
            flags |= TI_TYPE_FLAG_WRAP_ONLY;

        if (obj.via.sz == 5)
        {
            mp_obj_t mp_hid;

            if (mp_skip(up) != MP_STR || mp_next(up, &mp_hid) != MP_BOOL)
            {
                log_critical(
                    "task `new_type` for "TI_COLLECTION_ID" is invalid",
                    collection->id);
                return -1;
            }

            if (mp_hid.via.bool_)
                flags |= TI_TYPE_FLAG_WRAP_ONLY;
        }
    }

    if (mp_id.via.u64 >= TI_SPEC_ANY)
    {
        log_critical(
            "task `new_type` for "TI_COLLECTION_ID" is invalid; "
            "incorrect type id %"PRIu64,
            collection->id, mp_id.via.u64);
        return -1;
    }

    type = ti_type_create(
            collection->types,
            mp_id.via.u64,
            flags,
            mp_name.via.str.data,
            mp_name.via.str.n,
            mp_created.via.u64,
            0);  /* modified_at = 0 */

    if (!type)
    {
        log_critical(
            "task `new_type` for "TI_COLLECTION_ID" is invalid; "
            "cannot create type id %"PRIu64,
            collection->id, mp_id.via.u64);
        return -1;
    }

    return 0;
}

/*
 * The new_enum MUST be followed with set_enum_data to set the actual members
 * and methods. This creates place-holders for members to they can be used
 * while unpacking data.
 */
static int ctask__new_enum(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    mp_obj_t obj, mp_id, mp_name, mp_created, mp_size;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_size) != MP_U64)
    {
        log_critical(
            "task `new_enum` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }
    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (enum_)
    {
        log_critical(
                "task `new_enum` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" already found",
                collection->id, mp_id.via.u64);
        return -1;
    }
    enum_ = ti_enum_create(
            mp_id.via.u64,
            mp_name.via.str.data,
            mp_name.via.str.n,
            mp_created.via.u64,
            0);  /* modified_at = 0 */
    if (!enum_ || ti_enums_add(collection->enums, enum_))
    {
        log_critical(EX_MEMORY_S);
        goto fail0;
    }

    if (ti_enum_create_placeholders(enum_, mp_size.via.u64, &e))
    {
        log_critical(e.msg);
        goto fail1;
    }

    return 0;
fail1:
    ti_enums_del(collection->enums, enum_, NULL);
fail0:
    ti_enum_destroy(enum_);
    return -1;
}

static int ctask__set_enum_data(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    mp_obj_t obj, mp_id;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `set_enum_data` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `set_enum_data` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (ti_enum_set_members_from_vup(enum_, &vup, &e))
    {
        log_critical(e.msg);
        return -1;
    }

    if (ti_enum_init_methods_from_vup(enum_, &vup, &e))
    {
        log_critical(e.msg);
        return -1;
    }

    return 0;
}

static int ctask__import(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = 0;
    ex_t e = {0};
    mp_obj_t obj, mp_import_tasks;
    ti_collection_t * collection = thing->collection;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_import_tasks) != MP_BOOL ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `import` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    if (ti_collection_unpack(collection, up, &e))
    {
        log_error(e.msg);  /* not critical for sure */
        rc = -1;
    }

    if (!mp_import_tasks.via.bool_)
        ti_collection_tasks_clear(collection);

    return rc;
}

static int ctask__replace_root(ti_thing_t * thing, mp_unp_t * up)
{
    _Bool changed;
    ti_val_t * val;
    ti_collection_t * collection = thing->collection;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    /* drop the collection root thing; this must be removed as the packed
     * value might contain the same Id the the old collection root; when this
     * is applied as a change, we must prevent keeping the dropped thing; */
    changed = ti_changes_unkeep_dropped();
    ti_val_unsafe_drop((ti_val_t *) collection->root);
    if (changed)
        ti_changes_keep_dropped();

    collection->root = NULL;

    /* collection must be empty at this point */
    assert(collection->things->n == 0);

    val = ti_val_from_vup(&vup);
    if (!val)
        return -1;  /* logging is done */

    if (!ti_val_is_thing(val))
    {
        log_critical(
                "task `replace_root` for "TI_COLLECTION_ID" is invalid; "
                "value is not a thing",
                collection->id);
        return -1;
    }

    collection->root = (ti_thing_t *) val;  /* no reference required */
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'enum_id':.., 'members':.. }
 *
 * Note: decided to `panic` in case of failures since it might mess up
 *       the database in case of failure.
 */
static int ctask__set_enum(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    mp_obj_t obj, mp_id, mp_name, mp_created;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || (obj.via.sz != 4 && obj.via.sz != 5) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `set_enum` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (enum_)
    {
        log_critical(
                "task `set_enum` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" already found",
                collection->id, mp_id.via.u64);
        return -1;
    }
    else
    {
        enum_ = ti_enum_create(
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n,
                mp_created.via.u64,
                0);  /* modified_at = 0 */

        if (!enum_ || ti_enums_add(collection->enums, enum_))
        {
            log_critical(EX_MEMORY_S);
            goto fail0;
        }
    }

    if (ti_enum_init_members_from_vup(enum_, &vup, &e))
    {
        log_critical(e.msg);
        goto fail1;
    }

    if (obj.via.sz == 5 && ti_enum_init_methods_from_vup(enum_, &vup, &e))
    {
        log_critical(e.msg);
        goto fail1;
    }

    return 0;

fail1:
    ti_enums_del(collection->enums, enum_, NULL);
fail0:
    ti_enum_destroy(enum_);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'type_id':.., 'fields':.. }
 *
 * Note: decided to `panic` in case of failures since it might mess up
 *       the database in case of failure.
 */
static int ctask__set_type(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_modified;

    /* TODO: (COMPAT) For compatibility with versions before v0.9.6 */
    /* TODO: (COMPAT) For compatibility with versions before v0.9.23 */
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz < 3 || obj.via.sz > 6 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "task `set_type` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `set_type` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (type->fields->n)
    {
        log_critical(
                "task `set_type` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" is already initialized with fields",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (ti_type_init_from_unp(
            type, up,
            &e,
            obj.via.sz >= 4,
            obj.via.sz >= 5,
            obj.via.sz == 6))
    {
        log_critical(
            "task `set_type` for "TI_COLLECTION_ID" has failed; "
            "%s; remove type `%s`...",
            collection->id, e.msg, type->name);
        (void) ti_type_del(type, NULL);
        return -1;
    }

    /* update modified time-stamp */
    type->modified_at = mp_modified.via.u64;

    /* clean mappings */
    ti_type_map_cleanup(type);

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__to_thing(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;

    if (mp_skip(up) != MP_BOOL)
    {
        log_critical(
            "task `to_thing` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    if (ti_thing_is_instance(thing))
        ti_thing_t_to_object(thing);
    else
        log_error("thing "TI_THING_ID" is not an instance", thing->id);

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__to_type(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t mp_type_id;

    if (mp_next(up, &mp_type_id) != MP_U64)
    {
        log_critical(
            "task `to_type` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_type_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `to_type` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_type_id.via.u64);
        return -1;
    }

    if (ti_type_convert(type, thing, NULL, &e))
    {
        log_critical(
            "task `to_type` for "TI_COLLECTION_ID" has failed; %s",
            collection->id, e.msg);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__thing_restrict(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    uint16_t spec;
    mp_obj_t mp_spec;

    if (mp_next(up, &mp_spec) != MP_U64)
    {
        log_critical(
            "task `thing_restrict` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    spec = mp_spec.via.u64;
    thing->via.spec = spec;
    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__thing_remove(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t obj, mp_key;
    size_t i;

    if (mp_next(up, &obj) != MP_ARR)
    {
        log_critical(
            "task `thing_remove` for "TI_COLLECTION_ID" is invalid",
            collection->id);
        return -1;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_key) != MP_STR)
        {
            log_critical(
                    "task `thing_remove` from "TI_THING_ID": "
                    "invalid property data",
                    thing->id);
            continue;
        }
        ti_thing_o_del(thing, mp_key.via.str.data, mp_key.via.str.n);
    }
    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_enum_add(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_enum_t * enum_;
    ti_name_t * name;
    ti_val_t * val = NULL;
    mp_obj_t obj, mp_id, mp_name, mp_modified;
    int rc = -1;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `mod_enum_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->id);
        return rc;
    }

    enum_ = ti_enums_by_id(vup.collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `mod_enum_add` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                vup.collection->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(EX_MEMORY_S);
        goto fail0;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
        goto fail0;

    if (ti_val_is_closure(val))
    {
        if (ti_enum_add_method(enum_, name, (ti_closure_t *) val, &e))
        {
            log_critical(e.msg);
            goto fail0;
        }
    }
    else if (!ti_member_create(enum_, name, val, &e))
    {
        log_critical(e.msg);
        goto fail0;
    }

    /* update modified time-stamp */
    enum_->modified_at = mp_modified.via.u64;

    rc = 0;

fail0:
    ti_val_drop(val);
    ti_name_drop(name);
    return rc;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_enum_def(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_member_t * member;
    mp_obj_t obj, mp_id, mp_index, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_index) != MP_U64)
    {
        log_critical(
                "task `mod_enum_def` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `mod_enum_def` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_idx(enum_, mp_index.via.u64);
    if (!member)
    {
        log_critical(
                "task `mod_enum_def` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %u; index %"PRIu64" out of range",
                collection->id, enum_->enum_id, mp_index.via.u64);
        return -1;
    }

    /* update modified time-stamp */
    enum_->modified_at = mp_modified.via.u64;

    ti_member_def(member);

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_enum_del(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_member_t * member;
    mp_obj_t obj, mp_id, mp_name, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_name) != MP_STR && mp_name.tp != MP_U64))
    {
        log_critical(
                "task `mod_enum_del` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `mod_enum_del` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (mp_name.tp == MP_U64)
    {
        /* TODO (COMPAT):
         *   mp_name as MP_U64 is for compatibility with < 1.4.15 */
        member = ti_enum_member_by_idx(enum_, mp_name.via.u64);
        if (!member)
            goto not_found;

        ti_member_del(member);
    }
    else
    {
        member = ti_enum_member_by_strn(
                enum_,
                mp_name.via.str.data,
                mp_name.via.str.n);
        if (member)
        {
            ti_member_del(member);
        }
        else
        {
            ti_name_t * name = ti_names_weak_get_strn(
                    mp_name.via.str.data,
                    mp_name.via.str.n);

            if (!name)
                goto not_found;

            ti_enum_remove_method(enum_, name);
        }
    }

    /* update modified time-stamp */
    enum_->modified_at = mp_modified.via.u64;

    return 0;

not_found:
    log_critical(
            "task `mod_enum_del` for "TI_COLLECTION_ID" is invalid; "
            "enum with id %u; member or method not found",
            collection->id, enum_->enum_id);
    return -1;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_enum_mod(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_val_t * val;
    mp_obj_t obj, mp_id, mp_name, mp_modified;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    /* TODO (COMPAT): mp_name as MP_U64 is for compatibility with < 1.4.15 */
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_name) != MP_STR && mp_name.tp != MP_U64) ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
        return -1;  /* logging is done */

    if (ti_val_is_closure(val))
    {
        ti_method_t * method;
        ti_name_t * name;

        if (mp_name.tp == MP_U64)
        {
            log_critical(
                    "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid",
                    collection->id);
            return -1;
        }

        name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
        if (!name)
        {
            log_critical(
                    "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                    "enum with id %"PRIu64"; name is missing",
                    collection->id, mp_id.via.u64);
            return -1;
        }

        method = ti_enum_get_method(enum_, name);
        if (!method)
        {
            log_critical(
                    "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                    "enum with id %"PRIu64"; member or method not found",
                    collection->id, mp_id.via.u64);
            return -1;
        }

        ti_method_set_closure(method, (ti_closure_t *) val);
    }
    else
    {
        ti_member_t * member = mp_name.tp == MP_U64
                ? ti_enum_member_by_idx(enum_, mp_name.via.u64)
                : ti_enum_member_by_strn(
                    enum_,
                    mp_name.via.str.data,
                    mp_name.via.str.n);
        if (!member)
        {
            log_critical(
                    "task `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                    "enum with id %u; member not found",
                    collection->id, enum_->enum_id);
            return -1;
        }
        (void) ti_member_set_value(member, val, &e);
    }

    ti_val_drop(val);

    if (e.nr)
        log_critical(e.msg);
    else
        /* update modified time-stamp */
        enum_->modified_at = mp_modified.via.u64;

    return e.nr;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_enum_ren(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_member_t * member;

    mp_obj_t obj, mp_id, mp_name, mp_modified, mp_to;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        (mp_next(up, &mp_name) != MP_STR && mp_name.tp != MP_U64) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_to) != MP_STR)
    {
        log_critical(
                "task `mod_enum_ren` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (mp_name.tp == MP_U64)
    {
        /* TODO (COMPAT): For Compatibility with < 1.4.15 */
        member = ti_enum_member_by_idx(enum_, mp_name.via.u64);
        if (member)
            goto set_member;

        log_critical(
                "task `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64"; member not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_strn(
            enum_,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (member)
        goto set_member;
    else
    {
        ti_method_t * method;
        ti_name_t * name = ti_names_weak_get_strn(
                mp_name.via.str.data,
                mp_name.via.str.n);

        if (!name)
        {
            log_critical(
                    "task `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                    "enum with id %"PRIu64"; name is missing",
                    collection->id, mp_id.via.u64);
            return -1;
        }

        method = ti_enum_get_method(enum_, name);
        if (!method)
        {
            log_critical(
                    "task `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                    "enum with id %"PRIu64"; member or method not found",
                    collection->id, mp_id.via.u64);
            return -1;
        }

        (void) ti_method_set_name_e(
                method,
                enum_,
                mp_to.via.str.data,
                mp_to.via.str.n,
                &e);
        goto done;
    }

set_member:
    (void) ti_member_set_name(
            member,
            mp_to.via.str.data,
            mp_to.via.str.n,
            &e);

done:
    if (e.nr)
        log_critical(e.msg);
    else
        /* update modified time-stamp */
        enum_->modified_at = mp_modified.via.u64;

    return e.nr;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_add(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_type_t * type;
    ti_name_t * name;
    ti_raw_t * spec_raw;
    ti_field_t * field;
    ti_val_t * val = NULL;
    mp_obj_t obj, mp_id, mp_name, mp_target, mp_spec, mp_modified;
    int rc = -1;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || (obj.via.sz != 5 && obj.via.sz != 4) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_next(up, &mp_target) != MP_STR)  /* spec or method */
    {
        log_critical(
                "task `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->id);
        return rc;
    }

    type = ti_types_by_id(vup.collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                vup.collection->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(EX_MEMORY_S);
        goto fail0;
    }

    if (mp_str_eq(&mp_target, "spec"))
    {
        if (mp_next(up, &mp_spec) != MP_STR)
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->id);
            goto fail0;
        }
    }
    else if (mp_str_eq(&mp_target, "closure"))
    {
        val = ti_val_from_vup(&vup);

        if (!val)
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading method",
                    vup.collection->id);
            goto fail0;
        }

        if (!ti_val_is_closure(val))
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "expecting closure as method but got `%s`",
                    vup.collection->id, ti_val_str(val));
            goto fail0;
        }

        if (ti_type_add_method(type, name, (ti_closure_t *) val, &e))
        {
            log_critical(e.msg);
            goto fail0;
        }

        ti_decref(name);
        ti_decref(val);

        return 0;
    }
    else
    {
        log_critical(
                "task `mod_type_add` for "TI_COLLECTION_ID" is invalid; "
                "expecting `spec` or `closure`",
                vup.collection->id);
        goto fail0;
    }

    if (mp_spec.via.str.n == 1 && mp_spec.via.str.data[0] == '#')
    {
        if (type->idname)
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "duplicate id field (#)",
                    vup.collection->id);
            /* we can recover but this must not happen */
            ti_name_unsafe_drop(type->idname);
        }
        type->idname = name;
        return 0;
    }

    if (obj.via.sz == 5)
    {
        if (mp_skip(up) != MP_STR )
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->id);
            goto fail0;
        }

        val = ti_val_from_vup(&vup);
        if (!val)
        {
            log_critical(
                    "task `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading initial value",
                    vup.collection->id);
            goto fail0;
        }
    }

    spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
    if (!spec_raw)
    {
        log_critical(EX_MEMORY_S);
        goto fail1;
    }

    field = ti_field_create(name, spec_raw, type, &e);
    if (!field)
    {
        log_critical(e.msg);
        goto fail1;
    }

    if (val && ti_field_init_things(field, &val))
    {
        ti_panic("unrecoverable state after `mod_type_add` task");
        goto fail1;
    }

    /* update modified time-stamp */
    type->modified_at = mp_modified.via.u64;

    /* clean mappings */
    ti_type_map_cleanup(type);

    rc = 0;

fail1:
    ti_val_drop((ti_val_t *) spec_raw);
fail0:
    ti_val_drop(val);
    ti_name_drop(name);
    return rc;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_del(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    ti_name_t * name;
    ti_field_t * field;
    ti_method_t * method;
    mp_obj_t obj, mp_id, mp_name, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "task `mod_type_del` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "task `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %u; name is missing",
                collection->id, type->type_id);
        return -1;
    }

    field = ti_field_by_name(type, name);
    if (field)
    {
        if (ti_field_del(field))
        {
            log_critical(EX_MEMORY_S);
            return -1;
        }

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        /* clean mappings */
        ti_type_map_cleanup(type);

        return 0;
    }

    method = ti_type_get_method(type, name);
    if (method)
    {
        ti_type_remove_method(type, name);

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        return 0;
    }

    if (type->idname == name)
    {
        ti_name_unsafe_drop(type->idname);
        type->idname = NULL;

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        return 0;
    }

    log_critical(
            "task `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
            "type `%s` has no property or method `%s`",
            collection->id, type->name, name->str);
    return -1;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_mod(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    ti_name_t * name;

    ti_raw_t * spec_raw;
    ti_val_t * val = NULL;
    mp_obj_t obj, mp_id, mp_name, mp_target, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_next(up, &mp_target) != MP_STR)
    {
        log_critical(
                "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (mp_str_eq(&mp_target, "spec"))
    {
        mp_obj_t mp_spec;
        ti_field_t * field = ti_field_by_name(type, name);

        if (!field)
        {
            log_critical(
                    "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                    "type `%s` has no property `%s`",
                    collection->id, type->name, name->str);
            return -1;
        }

        if (mp_next(up, &mp_spec) != MP_STR)
        {
            log_critical(
                    "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid",
                    collection->id);
            return -1;
        }

        spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
        if (!spec_raw)
        {
            log_critical(EX_MEMORY_S);
            return -1;
        }

        (void) ti_field_mod_force(field, spec_raw, &e);

        ti_val_drop((ti_val_t *) spec_raw);

        if (e.nr)
        {
            log_critical(e.msg);
            return -1;
        }

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        /* clean mappings */
        ti_type_map_cleanup(type);

        return 0;
    }

    if (mp_str_eq(&mp_target, "closure"))
    {
        ti_vup_t vup = {
                .isclient = false,
                .collection = collection,
                .up = up,
        };
        ti_method_t * method = ti_type_get_method(type, name);

        if (!method)
        {
            log_critical(
                    "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                    "type `%s` has no method `%s`",
                    collection->id, type->name, name->str);
            return -1;
        }

        val = ti_val_from_vup(&vup);
        if (!val)
        {
            log_critical(
                    "task `mod_type_mod` for "TI_COLLECTION_ID" has failed; "
                    "error reading method",
                    collection->id);
            return -1;
        }

        if (!ti_val_is_closure(val))
        {
            log_critical(
                    "task `mod_type_mod` for "TI_COLLECTION_ID" has failed; "
                    "expecting closure as method but got `%s`",
                    collection->id, ti_val_str(val));
            ti_val_drop(val);
            return -1;
        }

        ti_method_set_closure(method, (ti_closure_t *) val);
        ti_decref(val);

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        return 0;
    }

    log_critical(
            "task `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
            "expecting `spec` or `closure`",
            collection->id);

    return -1;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_rel_add(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = -1;
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type, * otype;
    ti_name_t * name, * oname;
    ti_field_t * field, * ofield;
    mp_obj_t obj, mp_modified, mp_id1, mp_name1, mp_id2, mp_name2;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id1) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name1) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id2) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name2) != MP_STR)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id1.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id1.via.u64);
        return rc;
    }

    otype = ti_types_by_id(collection->types, mp_id2.via.u64);
    if (!otype)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id2.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name1.via.str.data, mp_name1.via.str.n);
    if (!name)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->id, mp_id1.via.u64);
        return rc;
    }

    oname = ti_names_weak_get_strn(mp_name2.via.str.data, mp_name2.via.str.n);
    if (!oname)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->id, mp_id2.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->id, type->name, name->str);
        return rc;
    }

    ofield = ti_field_by_name(otype, oname);
    if (!ofield)
    {
        log_critical(
                "task `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->id, otype->name, oname->str);
        return rc;
    }

    if (ti_condition_field_rel_init(field, ofield, &e))
    {
        log_critical(e.msg);
        return rc;
    }

    if (ti_field_relation_make(field, ofield, NULL))
    {
        ti_panic("unrecoverable error");
        log_critical(EX_MEMORY_S);
        return rc;
    }

    type->modified_at = mp_modified.via.u64;
    otype->modified_at = mp_modified.via.u64;

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_rel_del(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = -1;
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    ti_name_t * name;
    ti_field_t * field, * ofield;
    mp_obj_t obj, mp_modified, mp_id, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "task `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "task `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "task `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->id, type->name, name->str);
        return rc;
    }

    if (!ti_field_has_relation(field))
    {
        log_critical(
                "task `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no relation for property `%s`",
                collection->id, type->name, name->str);
        return rc;
    }

    ofield = field->condition.rel->field;

    field->type->modified_at = mp_modified.via.u64;
    ofield->type->modified_at = mp_modified.via.u64;

    free(field->condition.rel);
    field->condition.rel = NULL;

    free(ofield->condition.rel);
    ofield->condition.rel = NULL;

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_ren(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = -1;
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    ti_name_t * name;
    ti_field_t * field;
    ti_method_t * method;
    mp_obj_t obj, mp_id, mp_name, mp_to, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_to) != MP_STR)
    {
        log_critical(
                "task `mod_type_ren` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "task `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (field)
    {
        if (ti_field_set_name(
                field,
                mp_to.via.str.data,
                mp_to.via.str.n,
                &e))
            log_critical(e.msg);
        else
            /* update modified time-stamp */
            type->modified_at = mp_modified.via.u64;

        /* clean mappings */
        ti_type_map_cleanup(type);

        return e.nr;
    }

    method = ti_type_get_method(type, name);
    if (method)
    {
        if (ti_method_set_name_t(
                method,
                type,
                mp_to.via.str.data,
                mp_to.via.str.n,
                &e))
            log_critical(e.msg);
        else
            /* update modified time-stamp */
            type->modified_at = mp_modified.via.u64;

        return e.nr;
    }

    if (type->idname == name)
    {
        if (ti_type_set_idname(
                type,
                mp_to.via.str.data,
                mp_to.via.str.n,
                &e))
            log_critical(e.msg);
        else
            /* update modified time-stamp */
            type->modified_at = mp_modified.via.u64;

        return e.nr;
    }

    log_critical(
            "task `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
            "type `%s` has no property or method `%s`",
            collection->id, type->name, name->str);

    return rc;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_wpo(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_modified, mp_wpo;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_wpo) != MP_BOOL)
    {
        log_critical(
                "task `mod_type_wpo` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_wpo` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    ti_type_set_wrap_only_mode(type, mp_wpo.via.bool_);

    type->modified_at = mp_modified.via.u64;

    return 0;
}

/*
 * Returns 0 on success
 */
static int ctask__mod_type_hid(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_modified, mp_hid;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_hid) != MP_BOOL)
    {
        log_critical(
                "task `mod_type_hid` for "TI_COLLECTION_ID" is invalid",
                collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `mod_type_hid` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    ti_type_set_hide_id(type, mp_hid.via.bool_);

    type->modified_at = mp_modified.via.u64;

    return 0;
}


/*
 * Returns 0 on success
 * - for example: 'prop'
 */
static int ctask__del(ti_thing_t * thing, mp_unp_t * up)
{
    assert(ti_thing_is_object(thing));

    mp_obj_t mp_key;

    if (mp_next(up, &mp_key) != MP_STR)
    {
        log_critical(
                "task `del` from "TI_THING_ID": "
                "missing property data",
                thing->id);
        return -1;
    }

    ti_thing_o_del(thing, mp_key.via.str.data, mp_key.via.str.n);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{key: nkey}'
 */
static int ctask__ren(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    mp_obj_t obj, mp_okey, mp_nkey;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_okey) != MP_STR ||
        mp_next(up, &mp_nkey) != MP_STR)
    {
        log_critical(
                "task `ren` for "TI_THING_ID": "
                "missing old or new key",
                thing->id);
        return -1;
    }

    if (ti_thing_o_ren(
            thing,
            mp_okey.via.str.data,
            mp_okey.via.str.n,
            mp_nkey.via.str.data,
            mp_nkey.via.str.n,
            &e))
    {
        log_critical("task `ren` on "TI_THING_ID": %s",
                thing->id,
                e.msg);
        return -1;
    }
    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'name'
 */
static int ctask__del_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    mp_obj_t mp_name;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "task `del_procedure` in "TI_COLLECTION_ID": "
                "missing procedure name",
                collection->id);
        return -1;
    }

    procedure = ti_procedures_by_strn(
            collection->procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `del_procedure` cannot find `%.*s` in "TI_COLLECTION_ID,
                mp_name.via.str.n, mp_name.via.str.data,
                collection->id);
        return -1;
    }

    (void) ti_procedures_pop(collection->procedures, procedure);

    ti_procedure_destroy(procedure);
    return 0;  /* success */
}

/*
 * Returns 0 on success
 * - for example: '{name: closure}'
 */
static int ctask__new_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    int rc;
    mp_obj_t obj, mp_name, mp_created;
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `new_procedure` for "TI_COLLECTION_ID": "
                "missing map or name",
                collection->id);
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    procedure = NULL;

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        !(procedure = ti_procedure_create(
                mp_name.via.str.data,
                mp_name.via.str.n,
                closure,
                mp_created.via.u64)))
        goto failed;

    rc = ti_procedures_add(collection->procedures, procedure);
    if (rc == 0)
    {
        ti_decref(closure);
        return 0;  /* success */
    }

    if (rc < 0)
        log_critical(EX_MEMORY_S);
    else
        log_critical(
                "task `new_procedure` for "TI_COLLECTION_ID": "
                "procedure `%s` already exists",
                collection->id,
                procedure->name->str);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{name: closure}'
 */
static int ctask__mod_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_name, mp_created;
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `mod_procedure` for "TI_COLLECTION_ID": "
                "missing map or name",
                collection->id);
        return -1;
    }

    procedure = ti_procedures_by_strn(
            collection->procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `mod_procedure` cannot find `%.*s` in "TI_COLLECTION_ID,
                mp_name.via.str.n, mp_name.via.str.data,
                collection->id);
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    if (!closure || !ti_val_is_closure((ti_val_t *) closure))
    {
        log_critical(
                "task `mod_procedure` invalid closure in "TI_COLLECTION_ID,
                collection->id);
        ti_val_drop((ti_val_t *) closure);
        return -1;
    }

    ti_procedure_mod(procedure, closure, mp_created.via.u64);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123, run_at: ...}'
 */
static int ctask__vtask_new(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_run_at, mp_user_id;
    ti_collection_t * collection = thing->collection;
    ti_vtask_t * vtask = NULL;
    ti_varr_t * varr = NULL;
    ti_closure_t * closure;
    ti_user_t * user;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 5 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_run_at) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `vtask_new` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    if (!user)
        user = vec_first(ti.users);

    closure = (ti_closure_t *) ti_val_from_vup(&vup);

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        mp_skip(up) != MP_STR ||
        vec_reserve(&collection->vtasks, 1))
        goto fail0;

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    vtask = ti_vtask_create(
                mp_id.via.u64,
                mp_run_at.via.u64,
                user,
                closure,
                NULL,
                varr->vec);
    if (!vtask)
        goto fail0;

    ti_collection_update_next_free_id(collection, vtask->id);
    (void) ti_tasks_append(&collection->vtasks, vtask);
    free(varr);
    ti_decref(closure);
    return 0;

fail0:
    log_critical(
            "task `vtask_new` for "TI_COLLECTION_ID" has failed",
            collection->id);
    ti_val_drop((ti_val_t *) varr);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123}'
 */
static int ctask__vtask_del(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_collection_t * collection = thing->collection;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `vtask_del` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    ti_vtask_del(mp_id.via.u64, collection);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{id: 123}'
 */
static int ctask__vtask_cancel(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_collection_t * collection = thing->collection;
    ti_vtask_t * vtask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `vtask_cancel` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    vtask = ti_vtask_by_id(collection->vtasks, mp_id.via.u64);
    if (vtask)
        ti_vtask_cancel(vtask);
    return 0;
}

static int ctask__vtask_finish(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_run_at;
    ti_collection_t * collection = thing->collection;
    ti_val_t * val;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_run_at) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `vtask_finish` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
        goto fail0;

    if (ti_val_is_nil(val))
    {
        ti_decref(val);
        val = (ti_val_t *) ti_verror_from_code(EX_SUCCESS);
    }
    else if (!ti_val_is_error(val))
        goto fail0;

    vtask = ti_vtask_by_id(collection->vtasks, mp_id.via.u64);
    if (!vtask)
        goto fail0;

    vtask->run_at = mp_run_at.via.u64;
    ti_tasks_vtask_finish(vtask);

    ti_val_drop((ti_val_t *) vtask->verr);
    vtask->verr = (ti_verror_t *) val;
    return 0;

fail0:
    log_critical(
            "task `vtask_finish` for "TI_COLLECTION_ID" has failed",
            collection->id);
    ti_val_drop(val);
    return -1;
}

static int ctask__vtask_set_args(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_collection_t * collection = thing->collection;
    ti_varr_t * varr;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `vtask_set_args` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    vtask = ti_vtask_by_id(collection->vtasks, mp_id.via.u64);
    if (vtask)
    {
        vec_t * tmp = vtask->args;
        vtask->args = varr->vec;
        varr->vec = tmp;
    }

    ti_val_unsafe_drop((ti_val_t *) varr);
    return 0;

fail0:
    log_critical(
            "task `vtask_set_args` for "TI_COLLECTION_ID" has failed",
            collection->id);
    ti_val_drop((ti_val_t *) varr);
    return -1;
}

static int ctask__vtask_set_owner(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_user_id;
    ti_collection_t * collection = thing->collection;
    ti_user_t * user;
    ti_vtask_t * vtask;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64)
    {
        log_critical(
                "task `vtask_set_owner` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    user = ti_users_get_by_id(mp_user_id.via.u64);
    vtask = ti_vtask_by_id(collection->vtasks, mp_id.via.u64);
    if (vtask)
    {
        ti_user_drop(vtask->user);
        vtask->user = user;
        ti_incref(user);
    }

    return 0;
}

static int ctask__vtask_set_closure(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id;
    ti_collection_t * collection = thing->collection;
    ti_closure_t * closure;
    ti_vtask_t * vtask;
    ti_vup_t vup = {
            .isclient = false,
            .collection = collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "task `vtask_set_closure` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->id);
        return -1;
    }

    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    if (!closure || !ti_val_is_closure((ti_val_t *) closure))
        goto fail0;

    vtask = ti_vtask_by_id(collection->vtasks, mp_id.via.u64);
    if (vtask && ti_vtask_set_closure(vtask, closure) == 0)
        return 0;

    ti_val_unsafe_drop((ti_val_t *) closure);

    if (!vtask)
        return 0;
    closure = NULL;

fail0:
    log_critical(
            "task `vtask_set_closure` for "TI_COLLECTION_ID" has failed",
            collection->id);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/* TODO (COMPAT): Obsolete task */
static int ctask__del_timer(mp_unp_t * up)
{
    log_warning("task `del_timer` is obsolete");
    mp_skip(up);
    return 0;
}

/* TODO (COMPAT): Obsolete task */
static int ctask__new_timer(mp_unp_t * up)
{
    log_warning("task `new_timer` is obsolete");
    mp_skip(up);
    return 0;
}

/* TODO (COMPAT): Obsolete task */
static int ctask__set_timer_args(mp_unp_t * up)
{
    log_warning("task `set_timer_args` is obsolete");
    mp_skip(up);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: enum_id
 */
static int ctask__del_enum(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t mp_id;
    ti_enum_t * enum_;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `del_enum` from collection "TI_COLLECTION_ID": "
                "expecting an integer enum id",
                collection->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `del_enum` from collection "TI_COLLECTION_ID": "
                "enum with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (enum_->refcount)
    {
        log_critical(
                "task `del_enum` from collection "TI_COLLECTION_ID": "
                "enum with id %u still has %"PRIu64" references",
                collection->id, mp_id.via.u64, enum_->refcount);
        return -1;
    }

    /* NOTE: members may have more than one reference at this point
     *       because they might be in use in stored closures as cached
     *       values.
     */

    ti_enums_del(collection->enums, enum_, NULL);
    ti_enum_destroy(enum_);

    return 0;
}


/*
 * Returns 0 on success
 * - for example: type_id
 */
static int ctask__del_type(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t mp_id;
    ti_type_t * type;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "task `del_type` from collection "TI_COLLECTION_ID": "
                "expecting an integer type id",
                collection->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `del_type` from collection "TI_COLLECTION_ID": "
                "type with id %"PRIu64" not found",
                collection->id, mp_id.via.u64);
        return -1;
    }

    if (type->refcount)
    {
        log_critical(
                "task `del_type` from collection "TI_COLLECTION_ID": "
                "type with id %u still has %"PRIu64" references",
                collection->id, mp_id.via.u64, type->refcount);
        return -1;
    }

    ti_type_del(type, NULL);
    return 0;
}


/*
 * Returns 0 on success
 * - for example: {'prop': [del_count, thing_ids...]}
 */
static int ctask__set_remove(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_val_t * val;
    mp_obj_t obj, mp_prop, mp_id;
    ti_thing_t * t;
    size_t i;
    ti_field_t * field, * ofield = NULL;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_prop) != MP_STR ||
        mp_next(up, &obj) != MP_ARR)
    {
        log_critical(
                "task `remove` from set on "TI_THING_ID": "
                "missing map, property or delete_count",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "task `remove` from set on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_set(val))
    {
        log_critical(
                "task `remove` from set on "TI_THING_ID": "
                "expecting a `"TI_VAL_SET_S"`, got `%s`",
                thing->id,
                ti_val_str(val));
        return -1;
    }

    if (ti_thing_is_instance(thing))
    {
        field = ((ti_vset_t *) val)->key_;
        if (field->condition.rel)
            ofield = field->condition.rel->field;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_id) != MP_U64)
        {
            log_critical(
                    "task `remove` from set on "TI_THING_ID": "
                    "error reading integer value for property",
                    thing->id);
            return -1;
        }

        t = ti_collection_find_thing(collection, mp_id.via.u64);

        if (!t || !ti_vset_pop((ti_vset_t *) val, t))
        {
            log_error(
                    "task `remove` from set on "TI_THING_ID": "
                    "missing thing: "TI_THING_ID,
                    thing->id,
                    mp_id.via.u64);
            continue;
        }

        if (ofield)
            ofield->condition.rel->del_cb(ofield, t, thing);

        ti_val_unsafe_drop((ti_val_t *) t);
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int ctask__rename_enum(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = 0;
    ti_enum_t * enum_;
    mp_obj_t obj, mp_id, mp_name;
    ti_raw_t * rname;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_enum`: invalid format");
        return -1;
    }

    enum_ = ti_enums_by_id(thing->collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "task `rename_enum`: enum id %"PRIu64" not found",
                mp_id.via.u64);
        return -1;
    }

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    if (!rname || ti_enums_rename(thing->collection->enums, enum_, rname))
    {
        log_critical(EX_MEMORY_S);
        rc = -1;
    }

    ti_val_drop((ti_val_t *) rname);
    return rc;
}

/*
 * Returns 0 on success
 * - for example: {'old':name, 'name':name}
 */
static int ctask__rename_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    mp_obj_t obj, mp_old, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_old) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_procedure`: invalid format");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            thing->collection->procedures,
            mp_old.via.str.data,
            mp_old.via.str.n);

    if (!procedure)
    {
        log_critical(
                "task `rename_procedure` cannot find `%.*s`",
                mp_old.via.str.n, mp_old.via.str.data);
        return -1;
    }

    return ti_procedures_rename(
            collection->procedures,
            procedure,
            mp_name.via.str.data,
            mp_name.via.str.n);
}

/*
 * Returns 0 on success
 * - for example: {'id':id, 'name':name}
 */
static int ctask__rename_type(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = 0;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_name;
    ti_raw_t * rname;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical("task `rename_type`: invalid format");
        return -1;
    }

    type = ti_types_by_id(thing->collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "task `rename_type`: type id %"PRIu64" not found",
                mp_id.via.u64);
        return -1;
    }

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    if (!rname || ti_type_rename(type, rname))
    {
        log_critical(EX_MEMORY_S);
        rc = -1;
    }

    ti_val_drop((ti_val_t *) rname);
    return rc;
}

/*
 * Returns 0 on success
 * - for example: {'prop': value}
 */
static int ctask__fill(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_val_t * val;
    ti_varr_t * varr;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    mp_obj_t obj, mp_prop;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "task `fill` on "TI_THING_ID": "
                "missing map or property",
                thing->id);
        return -1;
    }

    varr = (ti_varr_t *) ti_thing_val_by_strn(
            thing,
            mp_prop.via.str.data,
            mp_prop.via.str.n);
    if (!varr)
    {
        log_critical(
                "task `fill` array on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        log_critical(
                "task `fill` on "TI_THING_ID": "
                "expecting a `"TI_VAL_LIST_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) varr));
        return -1;
    }

    val = ti_val_from_vup(&vup);

    if (!val)  /* both <0 and >0 are not correct since we should have n values */
    {
        log_critical(
                "task `fill` array on "TI_THING_ID": "
                "error reading value for property",
                thing->id);
        return -1;
    }

    if (ti_val_varr_fill(varr, &val, &e))
    {
        log_critical(
                "task `fill` array on "TI_THING_ID": "
                "fill failed: %s",
                thing->id, e.msg);
    }

    ti_val_unsafe_drop(val);
    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'prop': [index, del_count, new_count, values...]}
 */
static int ctask__splice(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    size_t n, i, c, cur_n, new_n;
    ti_varr_t * varr;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    mp_obj_t obj, mp_prop, mp_i, mp_c;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||

        mp_next(up, &mp_prop) != MP_STR ||
        mp_next(up, &obj) != MP_ARR ||  obj.via.sz < 2 ||

        mp_next(up, &mp_i) != MP_U64 ||
        mp_next(up, &mp_c) != MP_U64)
    {
        log_critical(
                "task `splice` on "TI_THING_ID": "
                "missing map, property, index, delete_count or new_count",
                thing->id);
        return -1;
    }

    varr = (ti_varr_t *) ti_thing_val_by_strn(
            thing,
            mp_prop.via.str.data,
            mp_prop.via.str.n);
    if (!varr)
    {
        log_critical(
                "task `splice` array on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        log_critical(
                "task `splice` on "TI_THING_ID": "
                "expecting a `"TI_VAL_LIST_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) varr));
        return -1;
    }

    cur_n = varr->vec->n;
    i = mp_i.via.u64;
    c = mp_c.via.u64;
    n = obj.via.sz - 2;

    if (c > cur_n)
    {
        log_critical("splice: delete count `c` > current `n`");
        return -1;
    }

    new_n = cur_n + n - c;

    if (new_n > cur_n && vec_reserve(&varr->vec, new_n))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    for (ssize_t x = i, y = i + c; x < y; ++x)
        ti_val_unsafe_drop(VEC_get(varr->vec, x));

    memmove(
            varr->vec->data + i + n,
            varr->vec->data  + i + c,
            (cur_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    while(n--)
    {
        ti_val_t * val = ti_val_from_vup(&vup);

        if (!val)  /* both <0 and >0 are not correct since we should have n values */
        {
            log_critical(
                    "task `splice` array on "TI_THING_ID": "
                    "error reading value for property",
                    thing->id);
            return -1;
        }

        if (ti_val_varr_append(varr, &val, &e))
        {
            log_critical("task `splice` array on "TI_THING_ID": %s",
                    thing->id,
                    e.msg);
            ti_val_unsafe_drop(val);
            return -1;
        }
    }

    varr->vec->n = new_n;

    if (new_n < cur_n)
        (void) vec_may_shrink(&varr->vec);

    return 0;
}

/*
 * Returns 0 on success (index are written from high to low values)
 * - for example: {'prop': [index, index, ...]}
 */
int ctask__arr_remove(ti_thing_t * thing, mp_unp_t * up)
{
    ti_varr_t * varr;
    mp_obj_t obj, mp_prop, mp_idx;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_prop) != MP_STR ||
        mp_next(up, &obj) != MP_ARR)
    {
        log_critical(
                "task `arr_remove` on "TI_THING_ID": "
                "missing map, property, index, delete_count or new_count",
                thing->id);
        return -1;
    }

    varr = (ti_varr_t *) ti_thing_val_by_strn(
            thing,
            mp_prop.via.str.data,
            mp_prop.via.str.n);

    if (!varr)
    {
        log_critical(
                "task `arr_remove` array on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        log_critical(
                "task `arr_remove` on "TI_THING_ID": "
                "expecting a `"TI_VAL_LIST_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) varr));
        return -1;
    }

    for (size_t i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_idx) != MP_U64)
        {
            log_critical(
                    "task `arr_remove` array on "TI_THING_ID": "
                    "invalid index type",
                    thing->id);
            return -1;
        }
        ti_val_unsafe_drop(vec_remove(varr->vec, mp_idx.via.u64));
    }

    return 0;
}

/*
 * Unpacker should be at point 'task': ...
 */
int ti_ctask_run(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_task;
    if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 2 ||
        mp_next(up, &mp_task) != MP_U64)
    {
        if (obj.tp != MP_MAP || obj.via.sz != 1 ||
            mp_next(up, &mp_task) != MP_STR || mp_task.via.str.n < 3)
        {
            log_critical(
                    "collection task is not a `map` or `type` "
                    "for thing "TI_THING_ID" is missing", 0);
            return -1;
        }
        goto version_v0;
    }

    switch ((ti_task_enum) mp_task.via.u64)
    {
    case TI_TASK_SET:               return ctask__set(thing, up);
    case TI_TASK_DEL:               return ctask__del(thing, up);
    case TI_TASK_SPLICE:            return ctask__splice(thing, up);
    case TI_TASK_SET_ADD:           return ctask__set_add(thing, up);
    case TI_TASK_SET_REMOVE:        return ctask__set_remove(thing, up);
    case TI_TASK_DEL_COLLECTION:    break;
    case TI_TASK_DEL_ENUM:          return ctask__del_enum(thing, up);
    case TI_TASK_DEL_EXPIRED:       break;
    case TI_TASK_DEL_MODULE:        break;
    case TI_TASK_DEL_NODE:          break;
    case TI_TASK_DEL_PROCEDURE:     return ctask__del_procedure(thing, up);
    case _TI_TASK_DEL_TIMER:        return ctask__del_timer(up);
    case TI_TASK_DEL_TOKEN:         break;
    case TI_TASK_DEL_TYPE:          return ctask__del_type(thing, up);
    case TI_TASK_DEL_USER:          break;
    case TI_TASK_DEPLOY_MODULE:     break;
    case TI_TASK_GRANT:             break;
    case TI_TASK_MOD_ENUM_ADD:      return ctask__mod_enum_add(thing, up);
    case TI_TASK_MOD_ENUM_DEF:      return ctask__mod_enum_def(thing, up);
    case TI_TASK_MOD_ENUM_DEL:      return ctask__mod_enum_del(thing, up);
    case TI_TASK_MOD_ENUM_MOD:      return ctask__mod_enum_mod(thing, up);
    case TI_TASK_MOD_ENUM_REN:      return ctask__mod_enum_ren(thing, up);
    case TI_TASK_MOD_TYPE_ADD:      return ctask__mod_type_add(thing, up);
    case TI_TASK_MOD_TYPE_DEL:      return ctask__mod_type_del(thing, up);
    case TI_TASK_MOD_TYPE_MOD:      return ctask__mod_type_mod(thing, up);
    case TI_TASK_MOD_TYPE_REL_ADD:  return ctask__mod_type_rel_add(thing, up);
    case TI_TASK_MOD_TYPE_REL_DEL:  return ctask__mod_type_rel_del(thing, up);
    case TI_TASK_MOD_TYPE_REN:      return ctask__mod_type_ren(thing, up);
    case TI_TASK_MOD_TYPE_WPO:      return ctask__mod_type_wpo(thing, up);
    case TI_TASK_NEW_COLLECTION:    break;
    case TI_TASK_NEW_MODULE:        break;
    case TI_TASK_NEW_NODE:          break;
    case TI_TASK_NEW_PROCEDURE:     return ctask__new_procedure(thing, up);
    case _TI_TASK_NEW_TIMER:        return ctask__new_timer(up);
    case TI_TASK_NEW_TOKEN:         break;
    case TI_TASK_NEW_TYPE:          return ctask__new_type(thing, up);
    case TI_TASK_NEW_USER:          break;
    case TI_TASK_RENAME_COLLECTION: break;
    case TI_TASK_RENAME_ENUM:       return ctask__rename_enum(thing, up);
    case TI_TASK_RENAME_MODULE:     break;
    case TI_TASK_RENAME_PROCEDURE:  return ctask__rename_procedure(thing, up);
    case TI_TASK_RENAME_TYPE:       return ctask__rename_type(thing, up);
    case TI_TASK_RENAME_USER:       break;
    case TI_TASK_RESTORE:           break;
    case TI_TASK_REVOKE:            break;
    case TI_TASK_SET_ENUM:          return ctask__set_enum(thing, up);
    case TI_TASK_SET_MODULE_CONF:   break;
    case TI_TASK_SET_MODULE_SCOPE:  break;
    case TI_TASK_SET_PASSWORD:      break;
    case TI_TASK_SET_TIME_ZONE:     break;
    case _TI_TASK_SET_TIMER_ARGS:   return ctask__set_timer_args(up);
    case TI_TASK_SET_TYPE:          return ctask__set_type(thing, up);
    case TI_TASK_TO_TYPE:           return ctask__to_type(thing, up);
    case TI_TASK_CLEAR_USERS:       break;
    case TI_TASK_TAKE_ACCESS:       break;
    case TI_TASK_ARR_REMOVE:        return ctask__arr_remove(thing, up);
    case TI_TASK_THING_CLEAR:       return ctask__thing_clear(thing, up);
    case TI_TASK_ARR_CLEAR:         return ctask__arr_clear(thing, up);
    case TI_TASK_SET_CLEAR:         return ctask__set_clear(thing, up);
    case TI_TASK_VTASK_NEW:         return ctask__vtask_new(thing, up);
    case TI_TASK_VTASK_DEL:         return ctask__vtask_del(thing, up);
    case TI_TASK_VTASK_CANCEL:      return ctask__vtask_cancel(thing, up);
    case TI_TASK_VTASK_FINISH:      return ctask__vtask_finish(thing, up);
    case TI_TASK_VTASK_SET_ARGS:    return ctask__vtask_set_args(thing, up);
    case TI_TASK_VTASK_SET_OWNER:   return ctask__vtask_set_owner(thing, up);
    case TI_TASK_VTASK_SET_CLOSURE: return ctask__vtask_set_closure(thing, up);
    case TI_TASK_THING_RESTRICT:    return ctask__thing_restrict(thing, up);
    case TI_TASK_THING_REMOVE:      return ctask__thing_remove(thing, up);
    case TI_TASK_SET_DEFAULT_DEEP:  break;
    case TI_TASK_RESTORE_FINISHED:  break;
    case TI_TASK_TO_THING:          return ctask__to_thing(thing, up);
    case TI_TASK_MOD_TYPE_HID:      return ctask__mod_type_hid(thing, up);
    case TI_TASK_REN:               return ctask__ren(thing, up);
    case TI_TASK_FILL:              return ctask__fill(thing, up);
    case TI_TASK_MOD_PROCEDURE:     return ctask__mod_procedure(thing, up);
    case TI_TASK_NEW_ENUM:          return ctask__new_enum(thing, up);
    case TI_TASK_SET_ENUM_DATA:     return ctask__set_enum_data(thing, up);
    case TI_TASK_REPLACE_ROOT:      return ctask__replace_root(thing, up);
    case TI_TASK_IMPORT:            return ctask__import(thing, up);
    case TI_TASK_ROOM_SET_NAME:     return ctask__room_set_name(thing, up);
    case TI_TASK_WHITELIST_ADD:     break;
    case TI_TASK_WHITELIST_DEL:     break;
    }

    log_critical("unknown collection task: %"PRIu64, mp_task.via.u64);
    return -1;

version_v0:
    switch (*mp_task.via.str.data)
    {
    case 'a':
        if (mp_str_eq(&mp_task, "add"))
            return ctask__set_add(thing, up);
        break;
    case 'd':
        if (mp_str_eq(&mp_task, "del"))
            return ctask__del(thing, up);
        if (mp_str_eq(&mp_task, "del_timer"))
            return ctask__del_timer(up);
        if (mp_str_eq(&mp_task, "del_enum"))
            return ctask__del_enum(thing, up);
        if (mp_str_eq(&mp_task, "del_type"))
            return ctask__del_type(thing, up);
        if (mp_str_eq(&mp_task, "del_procedure"))
            return ctask__del_procedure(thing, up);
        break;
    case 'e':
        if (mp_str_eq(&mp_task, "event"))
            return mp_skip(up) == MP_ERR ? -1 : 0;
        break;
    case 'n':
        if (mp_str_eq(&mp_task, "new_timer"))
            return ctask__new_timer(up);
        if (mp_str_eq(&mp_task, "new_type"))
            return ctask__new_type(thing, up);
        if (mp_str_eq(&mp_task, "new_procedure"))
            return ctask__new_procedure(thing, up);
        break;
    case 'm':
        if (mp_str_eq(&mp_task, "mod_enum_add"))
            return ctask__mod_enum_add(thing, up);
        if (mp_str_eq(&mp_task, "mod_enum_def"))
            return ctask__mod_enum_def(thing, up);
        if (mp_str_eq(&mp_task, "mod_enum_del"))
            return ctask__mod_enum_del(thing, up);
        if (mp_str_eq(&mp_task, "mod_enum_mod"))
            return ctask__mod_enum_mod(thing, up);
        if (mp_str_eq(&mp_task, "mod_enum_ren"))
            return ctask__mod_enum_ren(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_add"))
            return ctask__mod_type_add(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_del"))
            return ctask__mod_type_del(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_mod"))
            return ctask__mod_type_mod(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_rel_add"))
            return ctask__mod_type_rel_add(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_rel_del"))
            return ctask__mod_type_rel_del(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_ren"))
            return ctask__mod_type_ren(thing, up);
        if (mp_str_eq(&mp_task, "mod_type_wpo"))
            return ctask__mod_type_wpo(thing, up);
        break;
    case 'r':
        if (mp_str_eq(&mp_task, "remove"))
            return ctask__set_remove(thing, up);
        if (mp_str_eq(&mp_task, "rename_enum"))
            return ctask__rename_enum(thing, up);
        if (mp_str_eq(&mp_task, "rename_procedure"))
            return ctask__rename_procedure(thing, up);
        if (mp_str_eq(&mp_task, "rename_type"))
            return ctask__rename_type(thing, up);
        break;
    case 's':
        if (mp_str_eq(&mp_task, "set"))
            return ctask__set(thing, up);
        if (mp_str_eq(&mp_task, "splice"))
            return ctask__splice(thing, up);
        if (mp_str_eq(&mp_task, "set_timer_args"))
            return ctask__set_timer_args(up);
        if (mp_str_eq(&mp_task, "set_enum"))
            return ctask__set_enum(thing, up);
        if (mp_str_eq(&mp_task, "set_type"))
            return ctask__set_type(thing, up);
        break;
    case 't':
        if (mp_str_eq(&mp_task, "to_type"))
            return ctask__to_type(thing, up);
    }

    log_critical(
            "unknown collection task: `%.*s`",
            mp_task.via.str.n, mp_task.via.str.data);
    return -1;
}
