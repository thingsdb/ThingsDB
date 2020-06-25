/*
 * ti/job.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/enums.inline.h>
#include <ti/field.h>
#include <ti/job.h>
#include <ti/member.inline.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/qbind.h>
#include <ti/raw.inline.h>
#include <ti/thing.inline.h>
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
    ti_vset_t * vset;
    ti_name_t * name;
    ti_thing_t * t;
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

    name = ti_names_weak_get(mp_prop.via.str.data, mp_prop.via.str.n);
    if (!name || !(vset = (ti_vset_t *) ti_thing_val_weak_get(thing, name)))
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_set((ti_val_t *) vset))
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
                "expecting a `"TI_VAL_SET_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) vset));
        return -1;
    }

    for (i = obj.via.sz; i--;)
    {
        t = (ti_thing_t *) ti_val_from_vup(&vup);

        if (!t || !ti_val_is_thing((ti_val_t *) t) || ti_vset_add(vset, t))
        {
            log_critical(
                    "job `add` to set on "TI_THING_ID": "
                    "error while reading value for property: `%s`",
                    thing->id,
                    name->str);
            ti_val_drop((ti_val_t *) t);
            return -1;
        }
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
    ti_name_t * name;
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

    if (!ti_name_is_valid_strn(mp_prop.via.str.data, mp_prop.via.str.n))
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "invalid property: `%.*s`",
                thing->id, (int) mp_prop.via.str.n, mp_prop.via.str.data);
        return -1;
    }

    name = ti_names_get(mp_prop.via.str.data, mp_prop.via.str.n);
    if (!name)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    val = ti_val_from_vup(&vup);
    if (!val)
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (ti_val_make_assignable(&val, thing, name, &e))
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "error making variable assignable: `%s`",
                thing->id,
                name->str,
                e.msg);
        goto fail;
    }

    if (ti_thing_is_object(thing))
    {
        if (!ti_thing_o_prop_set(thing, name, val))
        {
            log_critical(
                    "job `set` to "TI_THING_ID": "
                    "error setting property: `%s` (type: `%s`)",
                    thing->id,
                    name->str,
                    ti_val_str(val));
            goto fail;
        }
    }
    else
    {
        ti_thing_t_prop_set(
                thing,
                ti_field_by_name(ti_thing_type(thing), name),
                val);
    }

    return 0;

fail:
    ti_val_drop(val);
    ti_name_drop(name);
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

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
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

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
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

    if (ti_type_init_from_unp(type, up, &e))
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
    mp_obj_t obj, mp_id, mp_name, mp_spec, mp_modified;
    int rc = -1;
    ti_vup_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = up,
    };

    if (mp_next(up, &obj) != MP_MAP || (obj.via.sz != 4 && obj.via.sz != 5) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_spec) != MP_STR)
    {
        log_critical(
                "job `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->root->id);
        return rc;
    }

    /*
     * TODO: since version 0.9.2 (June 2020), the initial value is no longer
     *       optional but always added to the `mod_type_add` job.
     */
    if (obj.via.sz == 5)
    {
        if (mp_skip(up) != MP_STR )
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->root->id);
            return rc;
        }

        val = ti_val_from_vup(&vup);
        if (!val)
        {
            log_critical(
                    "job `mod_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading initial value",
                    vup.collection->root->id);
            return rc;
        }
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
    spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
    if (!name || !spec_raw)
    {
        log_critical(EX_MEMORY_S);
        goto fail0;
    }

    field = ti_field_create(name, spec_raw, type, &e);
    if (!field)
    {
        log_critical(e.msg);
        goto fail0;
    }

    if (val && ti_field_init_things(field, &val, ev_id))
    {
        ti_panic("unrecoverable state after `mod_type_add` job");
        goto fail0;
    }

    /* update modified time-stamp */
    type->modified_at = mp_modified.via.u64;

    /* clean mappings */
    ti_type_map_cleanup(type);

    rc = 0;

fail0:
    ti_val_drop(val);
    ti_val_drop((ti_val_t *) spec_raw);
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

    name = ti_names_weak_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %u; name is missing",
                collection->root->id, type->type_id);
        return -1;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mod_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return -1;
    }

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

/*
 * Returns 0 on success
 */
static int job__mod_type_mod(ti_thing_t * thing, mp_unp_t * up)
{
    int rc = -1;
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    ti_name_t * name;
    ti_field_t * field;
    ti_raw_t * spec_raw;
    mp_obj_t obj, mp_id, mp_name, mp_spec, mp_modified;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 4 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_modified) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_spec) != MP_STR)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mod_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return rc;
    }

    spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
    if (!spec_raw)
    {
        log_critical(EX_MEMORY_S);
        return rc;
    }

    if (ti_field_mod_force(field, spec_raw, &e))
    {
        log_critical(e.msg);
        goto fail0;
    }

    /* update modified time-stamp */
    type->modified_at = mp_modified.via.u64;

    /* clean mappings */
    ti_type_map_cleanup(type);

    rc = 0;
fail0:
    ti_val_drop((ti_val_t *) spec_raw);
    return rc;
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

    name = ti_names_weak_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mod_type_ren` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return rc;
    }

    (void) ti_field_set_name(field, mp_to.via.str.data, mp_to.via.str.n, &e);

    if (e.nr)
        log_critical(e.msg);
    else
        /* update modified time-stamp */
        type->modified_at = mp_modified.via.u64;

    /* clean mappings */
    ti_type_map_cleanup(type);

    return e.nr;
}

/*
 * Returns 0 on success
 * - for example: 'prop'
 */
static int job__del(ti_thing_t * thing, mp_unp_t * up)
{
    assert (ti_thing_is_object(thing));
    ti_name_t * name;
    mp_obj_t mp_prop;

    if (mp_next(up, &mp_prop) != MP_STR)
    {
        log_critical(
                "job `del` property from "TI_THING_ID": "
                "missing property data",
                thing->id);
        return -1;
    }

    /* the job is already validated so getting the name will most likely
     * succeed */
    name = ti_names_weak_get(mp_prop.via.str.data, mp_prop.via.str.n);
    if (!name || !ti_thing_o_del(thing, name))
    {
        if (ti_name_is_valid_strn(mp_prop.via.str.data, mp_prop.via.str.n))
        {
            log_critical(
                    "job `del` property from "TI_THING_ID": "
                    "thing has no property `%.*s`",
                    thing->id,
                    (int) mp_prop.via.str.n, mp_prop.via.str.data);
        }
        else
        {
            log_critical(
                    "job `del` property from "TI_THING_ID": "
                    "invalid name property",
                    thing->id);
        }

        return -1;
    }
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

    procedure = ti_procedures_pop_strn(
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

    ti_procedure_destroy(procedure);
    return 0;  /* success */
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
    ti_raw_t * rname;
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

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    closure = (ti_closure_t *) ti_val_from_vup(&vup);
    procedure = NULL;

    if (!rname || !closure || !ti_val_is_closure((ti_val_t *) closure) ||
        !(procedure = ti_procedure_create(rname, closure, mp_created.via.u64)))
        goto failed;

    rc = ti_procedures_add(&collection->procedures, procedure);
    if (rc == 0)
    {
        ti_decref(rname);
        ti_decref(closure);

        return 0;  /* success */
    }

    if (rc < 0)
        log_critical(EX_MEMORY_S);
    else
        log_critical(
                "job `new_procedure` for "TI_COLLECTION_ID": "
                "procedure `%.*s` already exists",
                collection->root->id,
                (int) procedure->name->n, (char *) procedure->name->data);

failed:
    ti_procedure_destroy(procedure);
    ti_val_drop((ti_val_t *) rname);
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
    ti_vset_t * vset;
    ti_name_t * name;
    mp_obj_t obj, mp_prop, mp_id;
    ti_thing_t * t;
    size_t i;

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

    name = ti_names_weak_get(mp_prop.via.str.data, mp_prop.via.str.n);
    if (!name || !(vset = (ti_vset_t *) ti_thing_val_weak_get(thing, name)))
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) mp_prop.via.str.n, (char *) mp_prop.via.str.data);
        return -1;
    }

    if (!ti_val_is_set((ti_val_t *) vset))
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "expecting a `"TI_VAL_SET_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) vset));
        return -1;
    }

    for (i = obj.via.sz; i--;)
    {
        if (mp_next(up, &mp_id) != MP_U64)
        {
            log_critical(
                    "job `remove` from set on "TI_THING_ID": "
                    "error reading integer value for property: `%s`",
                    thing->id,
                    name->str);
            return -1;
        }

        t = imap_get(collection->things, mp_id.via.u64);

        if (!t || !ti_vset_pop(vset, t))
        {
            log_error(
                    "job `remove` from set on "TI_THING_ID": "
                    "missing thing: "TI_THING_ID,
                    thing->id,
                    mp_id.via.u64);
            continue;
        }
        ti_val_drop((ti_val_t *) t);
    }

    return 0;
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
    ti_name_t * name;
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

    name = ti_names_weak_get(mp_prop.via.str.data, mp_prop.via.str.n);
    if (!name || !(varr = (ti_varr_t *) ti_thing_val_weak_get(thing, name)))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) mp_prop.via.str.n, mp_prop.via.str.data);
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

    if (new_n > cur_n && vec_resize(&varr->vec, new_n))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    for (ssize_t x = i, y = i + c; x < y; ++x)
        ti_val_drop(vec_get(varr->vec, x));

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
                    "error reading value for property: `%s`",
                    thing->id,
                    name->str);
            return -1;
        }

        if (ti_varr_append(varr, (void **) &val, &e))
        {
            log_critical("job `splice` array on "TI_THING_ID": %s",
                    thing->id,
                    e.msg);
            ti_val_drop(val);
            return -1;
        }
    }

    varr->vec->n = new_n;

    if (new_n < cur_n)
        (void) vec_shrink(&varr->vec);

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
        if (mp_str_eq(&mp_job, "mod_type_ren"))
            return job__mod_type_ren(thing, up);
        break;
    case 'r':
        if (mp_str_eq(&mp_job, "remove"))
            return job__remove(thing, up);
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
