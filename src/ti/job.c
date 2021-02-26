/*
 * ti/job.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/condition.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/job.h>
#include <ti/member.inline.h>
#include <ti/method.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/qbind.h>
#include <ti/raw.inline.h>
#include <ti/thing.inline.h>
#include <ti/timer.h>
#include <ti/timer.inline.h>
#include <ti/types.inline.h>
#include <ti/val.h>
#include <ti/val.inline.h>
#include <ti/varr.h>
#include <ti/vset.h>
#include <ti/vup.t.h>
#include <util/mpack.h>

/*
 * Returns 0 on success
 * - for example: {'prop': [new_count, values...]}
 */
static int job__add(ti_thing_t * thing, mp_unp_t * up)
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
                "job `add` to set on "TI_THING_ID": "
                "missing map, property or new_count",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_set(val))
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
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
                    "job `add` to set on "TI_THING_ID": "
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
 * - for example: {'prop':value}
 */
static int job__set(ti_thing_t * thing, mp_unp_t * up)
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
                "job `set` to "TI_THING_ID": "
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
                "job `set` to "TI_THING_ID": "
                "error reading value for property",
                thing->id);
        goto fail;
    }

    if (ti_thing_is_object(thing))
    {
        if (ti_val_make_assignable(&val, thing, key, &e))
        {
            log_critical(
                    "job `set` to "TI_THING_ID": "
                    "error making variable assignable: `%s`",
                    thing->id,
                    e.msg);
            goto fail;
        }

        if (ti_thing_o_set(thing, key, val))
        {
            log_critical(
                    "job `set` to "TI_THING_ID": "
                    "error setting property (type: `%s`)",
                    thing->id,
                    ti_val_str(val));
            goto fail;
        }
    }
    else
    {
        ti_field_t * field = ti_field_by_name(
                ti_thing_type(thing),
                (ti_name_t *) key);

        if (!field)
        {
            log_critical(
                    "job `set` to "TI_THING_ID": "
                    "cannot find field",
                    thing->id);
            goto fail;

        }

        if (ti_field_make_assignable(field, &val, thing, &e))
        {
            log_critical(
                    "job `set` to "TI_THING_ID": "
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
static int job__new_type(ti_thing_t * thing, mp_unp_t * up)
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
    if (mp_next(up, &obj) != MP_MAP || (obj.via.sz != 4 && obj.via.sz != 3) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
            "job `new_type` for "TI_COLLECTION_ID" is invalid",
            collection->root->id);
        return -1;
    }

    if (obj.via.sz == 4)
    {
        mp_obj_t mp_wo;

        if (mp_skip(up) != MP_STR || mp_next(up, &mp_wo) != MP_BOOL)
        {
            log_critical(
                "job `new_type` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
            return -1;
        }

        if (mp_wo.via.bool_)
            flags |= TI_TYPE_FLAG_WRAP_ONLY;
    }

    if (mp_id.via.u64 >= TI_SPEC_ANY)
    {
        log_critical(
            "job `new_type` for "TI_COLLECTION_ID" is invalid; "
            "incorrect type id %"PRIu64,
            collection->root->id, mp_id.via.u64);
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
            "job `new_type` for "TI_COLLECTION_ID" is invalid; "
            "cannot create type id %"PRIu64,
            collection->root->id, mp_id.via.u64);
        return -1;
    }

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'enum_id':.., 'members':.. }
 *
 * Note: decided to `panic` in case of failures since it might mess up
 *       the database in case of failure.
 */
static int job__set_enum(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    mp_obj_t obj, mp_id, mp_name, mp_created;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_created) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "job `set_enum` for "TI_COLLECTION_ID" is invalid",
            collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (enum_)
    {
        log_critical(
                "job `set_enum` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" already found",
                collection->root->id, mp_id.via.u64);
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

    if (ti_enum_init_from_vup(enum_, &vup, &e))
    {
        log_critical(e.msg);
        goto fail1;
    }

    /* update created time-stamp */
    enum_->created_at = mp_created.via.u64;

    return 0;

fail1:
    ti_enums_del(collection->enums, enum_);
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
static int job__set_type(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_modified;

    /* TODO: (COMPAT) For compatibility with versions before v0.9.6 */
    /* TODO: (COMPAT) For compatibility with versions before v0.9.23 */
    if (mp_next(up, &obj) != MP_MAP || (
            obj.via.sz != 5 &&
            obj.via.sz != 4 &&
            obj.via.sz != 3) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
            "job `sew_type` for "TI_COLLECTION_ID" is invalid",
            collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `set_type` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    if (type->fields->n)
    {
        log_critical(
                "job `set_type` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" is already initialized with fields",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    if (ti_type_init_from_unp(type, up, &e, obj.via.sz >= 4, obj.via.sz == 5))
    {
        log_critical(
            "job `set_type` for "TI_COLLECTION_ID" has failed; "
            "%s; remove type `%s`...",
            collection->root->id, e.msg, type->name);
        (void) ti_type_del(type);
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
static int job__mod_enum_add(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_enum_t * enum_;
    ti_name_t * name;
    ti_member_t * member;
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
                "job `mod_enum_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->root->id);
        return rc;
    }

    enum_ = ti_enums_by_id(vup.collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `mod_enum_add` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                vup.collection->root->id, mp_id.via.u64);
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

    member = ti_member_create(enum_, name, val, &e);
    if (!member)
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
static int job__mod_enum_def(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_enum_def` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `mod_enum_def` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_idx(enum_, mp_index.via.u64);
    if (!member)
    {
        log_critical(
                "job `mod_enum_def` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %u; index %"PRIu64" out of range",
                collection->root->id, enum_->enum_id, mp_index.via.u64);
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
static int job__mod_enum_del(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_enum_del` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `mod_enum_del` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_idx(enum_, mp_index.via.u64);
    if (!member)
    {
        log_critical(
                "job `mod_enum_del` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %u; index %"PRIu64" out of range",
                collection->root->id, enum_->enum_id, mp_index.via.u64);
        return -1;
    }

    /* update modified time-stamp */
    enum_->modified_at = mp_modified.via.u64;

    ti_member_del(member);

    return 0;
}

/*
 * Returns 0 on success
 */
static int job__mod_enum_mod(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_member_t * member;
    ti_val_t * val;
    mp_obj_t obj, mp_id, mp_index, mp_modified;
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
        mp_next(up, &mp_index) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "job `mod_enum_mod` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_idx(enum_, mp_index.via.u64);
    if (!member)
    {
        log_critical(
                "job `mod_enum_mod` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %u; index %"PRIu64" out of range",
                collection->root->id, enum_->enum_id, mp_index.via.u64);
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
        return -1;


    (void) ti_member_set_value(member, val, &e);

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
static int job__mod_enum_ren(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_enum_t * enum_;
    ti_member_t * member;
    mp_obj_t obj, mp_id, mp_index, mp_modified, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_index) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "job `mod_enum_ren` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    member = ti_enum_member_by_idx(enum_, mp_index.via.u64);
    if (!member)
    {
        log_critical(
                "job `mod_enum_ren` for "TI_COLLECTION_ID" is invalid; "
                "enum with id %u; index %"PRIu64" out of range",
                collection->root->id, enum_->enum_id, mp_index.via.u64);
        return -1;
    }

    (void) ti_member_set_name(
            member,
            mp_name.via.str.data,
            mp_name.via.str.n,
            &e);

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
static int job__mod_type_add(
        ti_thing_t * thing,
        mp_unp_t * up,
        uint64_t ev_id)
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
                "job `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->root->id);
        return rc;
    }

    type = ti_types_by_id(vup.collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                vup.collection->root->id, mp_id.via.u64);
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
                    "job `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->root->id);
            goto fail0;
        }
    }
    else if (mp_str_eq(&mp_target, "closure"))
    {
        val = ti_val_from_vup(&vup);

        if (!val)
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading method",
                    vup.collection->root->id);
            goto fail0;
        }

        if (!ti_val_is_closure(val))
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "expecting closure as method but got `%s`",
                    vup.collection->root->id, ti_val_str(val));
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
                "job `mod_type_add` for "TI_COLLECTION_ID" is invalid; "
                "expecting `spec` or `closure`",
                vup.collection->root->id);
        goto fail0;
    }

    if (obj.via.sz == 5)
    {
        if (mp_skip(up) != MP_STR )
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->root->id);
            goto fail0;
        }

        val = ti_val_from_vup(&vup);
        if (!val)
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading initial value",
                    vup.collection->root->id);
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

    if (val && ti_field_init_things(field, &val, ev_id))
    {
        ti_panic("unrecoverable state after `mod_type_add` job");
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
static int job__mod_type_del(
        ti_thing_t * thing,
        mp_unp_t * up,
        uint64_t ev_id)
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
                "job `mod_type_del` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %u; name is missing",
                collection->root->id, type->type_id);
        return -1;
    }

    field = ti_field_by_name(type, name);
    if (field)
    {
        if (ti_field_del(field, ev_id))
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

    method = ti_method_by_name(type, name);
    if (method)
    {
        ti_type_remove_method(type, name);

        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

        return 0;
    }

    log_critical(
            "job `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
            "type `%s` has no property or method `%s`",
            collection->root->id, type->name, name->str);
    return -1;
}

/*
 * Returns 0 on success
 */
static int job__mod_type_mod(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    if (mp_str_eq(&mp_target, "spec"))
    {
        mp_obj_t mp_spec;
        ti_field_t * field = ti_field_by_name(type, name);

        if (!field)
        {
            log_critical(
                    "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                    "type `%s` has no property `%s`",
                    collection->root->id, type->name, name->str);
            return -1;
        }

        if (mp_next(up, &mp_spec) != MP_STR)
        {
            log_critical(
                    "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid",
                    collection->root->id);
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
        ti_method_t * method = ti_method_by_name(type, name);

        if (!method)
        {
            log_critical(
                    "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                    "type `%s` has no method `%s`",
                    collection->root->id, type->name, name->str);
            return -1;
        }

        val = ti_val_from_vup(&vup);
        if (!val)
        {
            log_critical(
                    "job `mod_type_mod` for "TI_COLLECTION_ID" has failed; "
                    "error reading method",
                    collection->root->id);
            return -1;
        }

        if (!ti_val_is_closure(val))
        {
            log_critical(
                    "job `mod_type_mod` for "TI_COLLECTION_ID" has failed; "
                    "expecting closure as method but got `%s`",
                    collection->root->id, ti_val_str(val));
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
            "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
            "expecting `spec` or `closure`",
            collection->root->id);

    return -1;
}

/*
 * Returns 0 on success
 */
static int job__mod_type_rel_add(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id1.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id1.via.u64);
        return rc;
    }

    otype = ti_types_by_id(collection->types, mp_id2.via.u64);
    if (!otype)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id2.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name1.via.str.data, mp_name1.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id1.via.u64);
        return rc;
    }

    oname = ti_names_weak_get_strn(mp_name2.via.str.data, mp_name2.via.str.n);
    if (!oname)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id2.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return rc;
    }

    ofield = ti_field_by_name(otype, oname);
    if (!ofield)
    {
        log_critical(
                "job `mod_type_rel_add` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, otype->name, oname->str);
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
static int job__mod_type_rel_del(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return rc;
    }

    if (!ti_field_has_relation(field))
    {
        log_critical(
                "job `mod_type_rel_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no relation for property `%s`",
                collection->root->id, type->name, name->str);
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
static int job__mod_type_ren(ti_thing_t * thing, mp_unp_t * up)
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
                "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get_strn(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
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

    method = ti_method_by_name(type, name);
    if (method)
    {
        if (ti_method_set_name(
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

    log_critical(
            "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
            "type `%s` has no property or method `%s`",
            collection->root->id, type->name, name->str);

    return rc;
}

/*
 * Returns 0 on success
 */
static int job__mod_type_wpo(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_modified, mp_wo;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_wo) != MP_BOOL)
    {
        log_critical(
                "job `mod_type_wpo` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_wpo` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    ti_type_set_wrap_only_mode(type, mp_wo.via.bool_);

    type->modified_at = mp_modified.via.u64;

    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'prop'
 */
static int job__del(ti_thing_t * thing, mp_unp_t * up)
{
    assert (ti_thing_is_object(thing));

    mp_obj_t mp_prop;

    if (mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "job `del` property from "TI_THING_ID": "
                "missing property data",
                thing->id);
        return -1;
    }

    ti_thing_o_del(thing, mp_prop.via.str.data, mp_prop.via.str.n);

    return 0;
}

/*
 * Returns 0 on success
 * - for example: 'name'
 */
static int job__del_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    mp_obj_t mp_name;

    if (mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "job `del_procedure` in "TI_COLLECTION_ID": "
                "missing procedure name",
                collection->root->id);
        return -1;
    }

    procedure = ti_procedures_by_strn(
            collection->procedures,
            mp_name.via.str.data,
            mp_name.via.str.n);

    if (!procedure)
    {
        log_critical(
                "job `del_procedure` cannot find `%.*s` in "TI_COLLECTION_ID,
                (int) mp_name.via.str.n, mp_name.via.str.data,
                collection->root->id);
        return -1;
    }

    (void) ti_procedures_pop(collection->procedures, procedure);

    ti_procedure_destroy(procedure);
    return 0;  /* success */
}

/*
 * Returns 0 on success
 * - for example: 'id'
 */
static int job__del_timer(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t mp_id;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "job `del_timer` in "TI_COLLECTION_ID": "
                "missing timer id",
                collection->root->id);
        return -1;
    }

    for (vec_each(collection->timers, ti_timer_t, timer))
    {
        if (timer->id == mp_id.via.u64)
        {
            ti_timer_mark_del(timer);
            break;
        }
    }

    /*
     * For a timer event it may occur that a timer is already marked for
     * deletion and the timer may even be removed.
     */
    return 0;
}

/*
 * Returns 0 on success
 * - for example: '{name: closure}'
 */
static int job__new_procedure(ti_thing_t * thing, mp_unp_t * up)
{
    int rc;
    mp_obj_t obj, mp_name, mp_created;
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
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
                "job `new_procedure` for "TI_COLLECTION_ID": "
                "missing map or name",
                collection->root->id);
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
                "job `new_procedure` for "TI_COLLECTION_ID": "
                "procedure `%s` already exists",
                collection->root->id,
                procedure->name);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: '{...}'
 */
static int job__new_timer(ti_thing_t * thing, mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_name, mp_next_run, mp_repeat, mp_user_id;
    ti_collection_t * collection = thing->collection;
    ti_timer_t * timer = NULL;
    ti_varr_t * varr = NULL;
    ti_closure_t * closure;
    ti_name_t * name;
    ti_user_t * user;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 7 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) <= 0 ||
        (mp_name.tp != MP_NIL && mp_name.tp != MP_STR) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_next_run) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_repeat) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_user_id) != MP_U64 ||
        mp_skip(up) != MP_STR)
    {
        log_critical(
                "job `new_timer` for "TI_COLLECTION_ID": "
                "invalid data",
                collection->root->id);
        return -1;
    }

    name = mp_name.tp != MP_STR
            ? ti_names_get(mp_name.via.str.data, mp_name.via.str.n)
            : NULL;
    user = ti_users_get_by_id(mp_user_id.via.u64);
    closure = (ti_closure_t *) ti_val_from_vup(&vup);

    if (!closure || !ti_val_is_closure((ti_val_t *) closure) ||
        mp_skip(up) != MP_STR ||
        vec_reserve(&collection->timers, 1))
        goto fail0;

    varr = (ti_varr_t *) ti_val_from_vup(&vup);
    if (!varr || !ti_val_is_array((ti_val_t *) varr))
        goto fail0;

    timer = ti_timer_create(
                mp_id.via.u64,
                name,
                mp_next_run.via.u64,
                mp_repeat.via.u64,
                collection->root->id,
                user,
                closure,
                varr->vec);
    if (!timer)
        goto fail0;

    ti_update_next_thing_id(timer->id);
    VEC_push(collection->timers, timer);
    free(varr);
    ti_decref(closure);
    return 0;

fail0:
    log_critical(
            "job `new_timer` for "TI_COLLECTION_ID" has failed",
            collection->root->id);
    ti_val_drop((ti_val_t *) varr);
    ti_val_drop((ti_val_t *) name);
    ti_val_drop((ti_val_t *) closure);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: enum_id
 */
static int job__del_enum(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t mp_id;
    ti_enum_t * enum_;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "job `del_enum` from collection "TI_COLLECTION_ID": "
                "expecting an integer enum id",
                collection->root->id);
        return -1;
    }

    enum_ = ti_enums_by_id(collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `del_enum` from collection "TI_COLLECTION_ID": "
                "enum with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    if (enum_->refcount)
    {
        log_critical(
                "job `del_enum` from collection "TI_COLLECTION_ID": "
                "enum with id %u still has %"PRIu64" references",
                collection->root->id, mp_id.via.u64, enum_->refcount);
        return -1;
    }

    /* NOTE: members may have more than one reference at this point
     *       because they might be in use in stored closures as cached
     *       values.
     */

    ti_enums_del(collection->enums, enum_);
    ti_enum_destroy(enum_);

    return 0;
}


/*
 * Returns 0 on success
 * - for example: type_id
 */
static int job__del_type(ti_thing_t * thing, mp_unp_t * up)
{
    ti_collection_t * collection = thing->collection;
    mp_obj_t mp_id;
    ti_type_t * type;

    if (mp_next(up, &mp_id) != MP_U64)
    {
        log_critical(
                "job `del_type` from collection "TI_COLLECTION_ID": "
                "expecting an integer type id",
                collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `del_type` from collection "TI_COLLECTION_ID": "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return -1;
    }

    if (type->refcount)
    {
        log_critical(
                "job `del_type` from collection "TI_COLLECTION_ID": "
                "type with id %u still has %"PRIu64" references",
                collection->root->id, mp_id.via.u64, type->refcount);
        return -1;
    }

    ti_type_del(type);
    return 0;
}


/*
 * Returns 0 on success
 * - for example: {'prop': [del_count, thing_ids...]}
 */
static int job__remove(ti_thing_t * thing, mp_unp_t * up)
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
                "job `remove` from set on "TI_THING_ID": "
                "missing map, property or delete_count",
                thing->id);
        return -1;
    }

    val = ti_thing_val_by_strn(thing, mp_prop.via.str.data, mp_prop.via.str.n);
    if (!val)
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_set(val))
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
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
                    "job `remove` from set on "TI_THING_ID": "
                    "error reading integer value for property",
                    thing->id);
            return -1;
        }

        t = ti_collection_find_thing(collection, mp_id.via.u64);

        if (!t || !ti_vset_pop((ti_vset_t *) val, t))
        {
            log_error(
                    "job `remove` from set on "TI_THING_ID": "
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
static int job__rename_enum(ti_thing_t * thing, mp_unp_t * up)
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
        log_critical("job `rename_enum`: invalid format");
        return -1;
    }

    enum_ = ti_enums_by_id(thing->collection->enums, mp_id.via.u64);
    if (!enum_)
    {
        log_critical(
                "job `rename_enum`: enum id %"PRIu64" not found",
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
static int job__rename_procedure(ti_thing_t * thing, mp_unp_t * up)
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
        log_critical("job `rename_procedure`: invalid format");
        return -1;
    }

    procedure = ti_procedures_by_strn(
            thing->collection->procedures,
            mp_old.via.str.data,
            mp_old.via.str.n);

    if (!procedure)
    {
        log_critical(
                "job `rename_procedure` cannot find `%.*s`",
                (int) mp_old.via.str.n, mp_old.via.str.data);
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
static int job__rename_type(ti_thing_t * thing, mp_unp_t * up)
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
        log_critical("job `rename_type`: invalid format");
        return -1;
    }

    type = ti_types_by_id(thing->collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `rename_type`: type id %"PRIu64" not found",
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
 * - for example: {'prop': [index, del_count, new_count, values...]}
 */
static int job__splice(ti_thing_t * thing, mp_unp_t * up)
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
                "job `splice` on "TI_THING_ID": "
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
                "job `splice` array on "TI_THING_ID": "
                "missing property",
                thing->id);
        return -1;
    }

    if (!ti_val_is_list((ti_val_t *) varr))
    {
        log_critical(
                "job `splice` on "TI_THING_ID": "
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
                    "job `splice` array on "TI_THING_ID": "
                    "error reading value for property",
                    thing->id);
            return -1;
        }

        if (ti_varr_append(varr, (void **) &val, &e))
        {
            log_critical("job `splice` array on "TI_THING_ID": %s",
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
 * Unpacker should be at point 'job': ...
 */
int ti_job_run(ti_thing_t * thing, mp_unp_t * up, uint64_t ev_id)
{
    assert (thing);
    assert (thing->collection);
    mp_obj_t obj, mp_job;
    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_job) != MP_STR || mp_job.via.str.n < 3)
    {
        log_critical(
                "job is not a `map` or `type` "
                "for thing "TI_THING_ID" is missing", 0);
        return -1;
    }

    switch (*mp_job.via.str.data)
    {
    case 'a':
        if (mp_str_eq(&mp_job, "add"))
            return job__add(thing, up);
        break;
    case 'd':
        if (mp_str_eq(&mp_job, "del"))
            return job__del(thing, up);
        if (mp_str_eq(&mp_job, "job__del_timer"))
            return job__del_timer(thing, up);
        if (mp_str_eq(&mp_job, "del_enum"))
            return job__del_enum(thing, up);
        if (mp_str_eq(&mp_job, "del_type"))
            return job__del_type(thing, up);
        if (mp_str_eq(&mp_job, "del_procedure"))
            return job__del_procedure(thing, up);
        break;
    case 'e':
        if (mp_str_eq(&mp_job, "event"))
            return mp_skip(up) == MP_ERR ? -1 : 0;
        break;
    case 'n':
        if (mp_str_eq(&mp_job, "new_timer"))
            return job__new_timer(thing, up);
        if (mp_str_eq(&mp_job, "new_type"))
            return job__new_type(thing, up);
        if (mp_str_eq(&mp_job, "new_procedure"))
            return job__new_procedure(thing, up);
        break;
    case 'm':
        if (mp_str_eq(&mp_job, "mod_enum_add"))
            return job__mod_enum_add(thing, up);
        if (mp_str_eq(&mp_job, "mod_enum_def"))
            return job__mod_enum_def(thing, up);
        if (mp_str_eq(&mp_job, "mod_enum_del"))
            return job__mod_enum_del(thing, up);
        if (mp_str_eq(&mp_job, "mod_enum_mod"))
            return job__mod_enum_mod(thing, up);
        if (mp_str_eq(&mp_job, "mod_enum_ren"))
            return job__mod_enum_ren(thing, up);
        if (mp_str_eq(&mp_job, "mod_type_add"))
            return job__mod_type_add(thing, up, ev_id);
        if (mp_str_eq(&mp_job, "mod_type_del"))
            return job__mod_type_del(thing, up, ev_id);
        if (mp_str_eq(&mp_job, "mod_type_mod"))
            return job__mod_type_mod(thing, up);
        if (mp_str_eq(&mp_job, "mod_type_rel_add"))
            return job__mod_type_rel_add(thing, up);
        if (mp_str_eq(&mp_job, "mod_type_rel_del"))
            return job__mod_type_rel_del(thing, up);
        if (mp_str_eq(&mp_job, "mod_type_ren"))
            return job__mod_type_ren(thing, up);
        if (mp_str_eq(&mp_job, "mod_type_wpo"))
            return job__mod_type_wpo(thing, up);
        break;
    case 'r':
        if (mp_str_eq(&mp_job, "remove"))
            return job__remove(thing, up);
        if (mp_str_eq(&mp_job, "rename_enum"))
            return job__rename_enum(thing, up);
        if (mp_str_eq(&mp_job, "rename_procedure"))
            return job__rename_procedure(thing, up);
        if (mp_str_eq(&mp_job, "rename_type"))
            return job__rename_type(thing, up);
        break;
    case 's':
        if (mp_str_eq(&mp_job, "set"))
            return job__set(thing, up);
        if (mp_str_eq(&mp_job, "splice"))
            return job__splice(thing, up);
        if (mp_str_eq(&mp_job, "set_enum"))
            return job__set_enum(thing, up);
        if (mp_str_eq(&mp_job, "set_type"))
            return job__set_type(thing, up);
        break;
    }

    log_critical("unknown job: `%.*s`",
            (int) mp_job.via.str.n, mp_job.via.str.data);
    return -1;
}
