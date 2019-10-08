/*
 * ti/job.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/job.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/procedure.h>
#include <ti/procedures.h>
#include <ti/raw.inline.h>
#include <ti/syntax.h>
#include <ti/thing.inline.h>
#include <ti/types.inline.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>

/*
 * Returns 0 on success
 * - for example: {'prop': [new_count, values...]}
 */
static int job__add(ti_thing_t * thing, mp_unp_t * up)
{
    ti_vset_t * vset;
    ti_name_t * name;
    ti_thing_t * t;
    size_t i, m;
    mp_obj_t obj, mp_prop;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
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
    if (!name || !(vset = ti_thing_is_object(thing)
            ? (ti_vset_t *) ti_thing_o_val_weak_get(thing, name)
            : (ti_vset_t *) ti_thing_t_val_weak_get(thing, name)))
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

    for(i = 0, m = obj.via.sz; i < m; ++i)
    {
        t = (ti_thing_t *) ti_val_from_unp(&vup);

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
    ti_val_t * val;
    ti_name_t * name;
    mp_obj_t obj, mp_prop;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
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

    val = ti_val_from_unp(&vup);
    if (!val)
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
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
        ti_thing_t_prop_set(thing, name, val);
    }

    return 0;

fail:
    ti_val_drop(val);
    ti_name_drop(name);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'type_id':.., 'name':.. 'fields': {name: spec_raw,...}}
 *
 * Note: decided to `panic` in case of failures since it might mess up
 *       the database in case of failure.
 */
static int job__new_type(ti_thing_t * thing, mp_unp_t * up)
{
    ex_t e = {0};
    ti_collection_t * collection = thing->collection;
    ti_type_t * type;
    mp_obj_t obj, mp_id, mp_name;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR)
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

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        type = ti_type_create(
                collection->types,
                mp_id.via.u64,
                mp_name.via.str.data,
                mp_name.via.str.n);

        if (!type)
        {
            ti_type_drop(type);
            log_critical("memory allocation error");
            return -1;
        }
    }

    if (ti_type_init_from_unp(type, up, &e))
    {
        log_critical(
            "job `new_type` for "TI_COLLECTION_ID" has failed; "
            "%s; remove type `%s`...",
            collection->root->id, e.msg, type->name);
        (void) ti_type_del(type);
        return -1;
    }

    return 0;
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
    mp_obj_t obj, mp_id, mp_name, mp_spec;
    int rc = -1;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
    };

    if (mp_next(up, &obj) != MP_MAP || (obj.via.sz != 3 && obj.via.sz != 4) ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_spec) != MP_STR)
    {
        log_critical(
                "job `mode_type_add` for "TI_COLLECTION_ID" is invalid",
                vup.collection->root->id);
        return rc;
    }

    if (obj.via.sz == 4)
    {
        if (mp_skip(up) != MP_STR )
        {
            log_critical(
                    "job `mode_type_add` for "TI_COLLECTION_ID" is invalid",
                    vup.collection->root->id);
            return rc;
        }

        val = ti_val_from_unp(&vup);
        if (!val)
        {
            log_critical(
                    "job `mode_type_add` for "TI_COLLECTION_ID" has failed; "
                    "error reading initial value",
                    vup.collection->root->id);
            return rc;
        }
    }

    type = ti_types_by_id(vup.collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mode_type_add` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                vup.collection->root->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_get(mp_name.via.str.data, mp_name.via.str.n);
    spec_raw = ti_str_create(mp_spec.via.str.data, mp_spec.via.str.n);
    if (!name || !spec_raw)
    {
        log_critical(EX_MEMORY_S);
        goto done;
    }

    field = ti_field_create(name, spec_raw, type, &e);
    if (!field)
    {
        log_critical(e.msg);
        goto done;
    }

    if (val && ti_field_init_things(field, &val, ev_id))
    {
        log_critical(EX_MEMORY_S);
        goto done;
    }

    rc = 0;

done:
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
    uint16_t type_id;

    mp_obj_t obj, mp_id, mp_name, mp_spec;
    int rc = -1;

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 2 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "job `mode_type_del` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return -1;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mode_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %u not found",
                collection->root->id, type_id);
        return -1;
    }

    name = ti_names_weak_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mode_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type with id %u; name is missing",
                collection->root->id, type_id);
        return -1;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mode_type_del` for "TI_COLLECTION_ID" is invalid; "
                "type `%s` has no property `%s`",
                collection->root->id, type->name, name->str);
        return -1;
    }

    if (ti_field_del(field, ev_id))
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

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
    mp_obj_t obj, mp_id, mp_name, mp_spec;
    int rc = -1;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 3 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_name) != MP_STR ||
        mp_skip(up) != MP_STR ||
        mp_next(up, &mp_spec) != MP_STR)
    {
        log_critical(
                "job `mode_type_mod` for "TI_COLLECTION_ID" is invalid",
                collection->root->id);
        return rc;
    }

    type = ti_types_by_id(collection->types, mp_id.via.u64);
    if (!type)
    {
        log_critical(
                "job `mode_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64" not found",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    name = ti_names_weak_get(mp_name.via.str.data, mp_name.via.str.n);
    if (!name)
    {
        log_critical(
                "job `mode_type_mod` for "TI_COLLECTION_ID" is invalid; "
                "type with id %"PRIu64"; name is missing",
                collection->root->id, mp_id.via.u64);
        return rc;
    }

    field = ti_field_by_name(type, name);
    if (!field)
    {
        log_critical(
                "job `mode_type_mod` for "TI_COLLECTION_ID" is invalid; "
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

    if (ti_field_mod(field, spec_raw, 0, &e))
    {
        log_critical(e.msg);
        goto done;
    }

    rc = 0;
done:
    ti_val_drop((ti_val_t *) spec_raw);
    return rc;
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
    mp_obj_t obj, mp_name;
    ti_collection_t * collection = thing->collection;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_raw_t * rname;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
    };

    if (mp_next(up, &obj) != MP_MAP || obj.via.sz != 1 ||
        mp_next(up, &mp_name) != MP_STR)
    {
        log_critical(
                "job `new_procedure` for "TI_COLLECTION_ID": "
                "missing map or name",
                collection->root->id);
        return -1;
    }

    rname = ti_str_create(mp_name.via.str.data, mp_name.via.str.n);
    closure = (ti_closure_t *) ti_val_from_unp(&vup);
    procedure = NULL;

    if (!rname || !closure || !ti_val_is_closure((ti_val_t *) closure) ||
        !(procedure = ti_procedure_create(rname, closure)))
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
    size_t i, m;

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
    if (!name || !(vset = ti_thing_is_object(thing)
            ? (ti_vset_t *) ti_thing_o_val_weak_get(thing, name)
            : (ti_vset_t *) ti_thing_t_val_weak_get(thing, name)))
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

    for (i = 0, m = obj.via.sz; i < m; ++i)
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
    ti_collection_t * collection = thing->collection;
    ti_val_unp_t vup = {
            .isclient = false,
            .collection = thing->collection,
            .up = &up,
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
    if (!name || !(varr = ti_thing_is_object(thing)
            ? (ti_varr_t *) ti_thing_o_val_weak_get(thing, name)
            : (ti_varr_t *) ti_thing_t_val_weak_get(thing, name)))
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
                "expecting a `"TI_VAL_ARR_LIST_S"`, got `%s`",
                thing->id,
                ti_val_str((ti_val_t *) varr));
        return -1;
    }

    cur_n = varr->vec->n;
    i = mp_i.via.u64;
    c = mp_c.via.u64;
    n = obj.via.sz - 1;

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
        ti_val_t * val = ti_val_from_unp(&vup);

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
    mp_obj_t mp_job;
    if (mp_next(up, &mp_job) != MP_STR || mp_job.via.str.n < 3)
    {
        log_critical(
                "job `type` for thing "TI_THING_ID" is missing",
                thing->id);
        return -1;
    }

    switch (*mp_job.via.str.data)
    {
    case 'a':
        return job__add(thing, up);
    case 'd':
        switch (mp_job.via.str.n)
        {
        case 3: return job__del(thing, up);
        case 8: return job__del_type(thing, up);
        }
        return job__del_procedure(thing, up);
    case 'n':
        return mp_job.via.str.n == 8
                ? job__new_type(thing, up)
                : job__new_procedure(thing, up);
    case 'm':
        if (mp_job.via.str.n == 12) switch (mp_job.via.str.data[9])
        {
        case 'a': return job__mod_type_add(thing, up, ev_id);
        case 'd': return job__mod_type_del(thing, up, ev_id);
        case 'm': return job__mod_type_mod(thing, up);
        }
        break;
    case 'r':
        return job__remove(thing, up);
    case 's':
        return mp_job.via.str.n == 3
                ? job__set(thing, up)
                : job__splice(thing, up);
    }

    log_critical("unknown job: `%.*s`",
            (int) mp_job.via.str.n, mp_job.via.str.data);
    return -1;
}
