/*
 * ti/job.c
 */
#include <assert.h>
#include <ti/job.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/names.h>

static int job__assign(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__del(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__push(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__set(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__unset(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
/*
 * (Log function)
 * Unpacker should be at point 'job': ...
 */
int ti_job_run(ti_collection_t * collection, ti_thing_t * thing, qp_unpacker_t * unp)
{
    qp_obj_t qp_job_name;
    const uchar * raw;
    if (!qp_is_raw(qp_next(unp, &qp_job_name)) || qp_job_name.len < 2)
    {
        log_critical(
                "job `type` for thing "TI_THING_ID" is missing",
                thing->id);
        return -1;
    }

    raw = qp_job_name.via.raw;

    switch (*raw)
    {
    case 'a':
        return job__assign(collection, thing, unp);
    case 'd':
        return job__del(collection, thing, unp);
    case 'p':
        return job__push(collection, thing, unp);
    case 's':
        return job__set(collection, thing, unp);
    case 'u':
        return job__unset(collection, thing, unp);
    }

    log_critical("unknown job: `%.*s`", (int) qp_job_name.len, (char *) raw);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: {'prop':value}
 */
static int job__assign(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ti_val_t val;
    ti_name_t * name;
    qp_obj_t qp_prop;
    if (!qp_is_map(qp_next(unp, NULL)) || !qp_is_raw(qp_next(unp, &qp_prop)))
    {
        log_critical(
                "job `assign` to "TI_THING_ID": "
                "missing map or property",
                thing->id);
        return -1;
    }

    if (!ti_name_is_valid_strn((const char *) qp_prop.via.raw, qp_prop.len))
    {
        log_critical(
                "job `assign` to "TI_THING_ID": "
                "invalid property: `%.*s`",
                thing->id,
                (int) qp_prop.len,
                (const char *) qp_prop.via.raw);
        return -1;
    }

    name = ti_names_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (ti_val_from_unp(&val, unp, collection->things))
    {
        log_critical(
                "job `assign` to "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (ti_thing_weak_setv(thing, name, &val))
    {
        log_critical(
                "job `assign` to "TI_THING_ID": "
                "error setting property: `%s` (type: `%s`)",
                thing->id,
                name->str,
                ti_val_str(&val));
        goto fail;
    }

    return 0;

fail:
    ti_val_clear(&val);
    ti_name_drop(name);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: 'prop'
 */
static int job__del(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    qp_obj_t qp_prop;
    ti_name_t * name;
    if (!qp_is_raw(qp_next(unp, &qp_prop)))
    {
        log_critical(
                "job `del` property from "TI_THING_ID": "
                "missing property data",
                thing->id);
        return -1;
    }

    /* the job is already validated so getting the name will most likely
     * succeed */
    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !ti_thing_del(thing, name))
    {
        if (ti_name_is_valid_strn((const char *) qp_prop.via.raw, qp_prop.len))
        {
            log_critical(
                    "job `del` property from "TI_THING_ID": "
                    "thing has no property `%.*s`",
                    thing->id,
                    (int) qp_prop.len, (const char *) qp_prop.via.raw);
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
 * - for example: {'prop': [values]}
 */
static int job__push(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ex_t * e = ex_use();
    int rc = 0;
    size_t grow_sz = 6;
    ssize_t n;
    qp_types_t tp;
    ti_val_t val, * arr;
    ti_name_t * name;
    qp_obj_t qp_prop;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_prop)) ||
        !qp_is_array((tp = qp_next(unp, NULL))))
    {
        log_critical(
                "job `push` to array on "TI_THING_ID": "
                "missing map, property or array of values",
                thing->id);
        return -1;
    }

    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !(arr = ti_thing_get(thing, name)))
    {
        log_critical(
                "job `push` to array on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_prop.len, (char *) qp_prop.via.raw);
        return -1;
    }

    if (!ti_val_is_mutable_arr(arr))
    {
        log_critical(
                "job `push` to array on "TI_THING_ID": "
                "expecting a mutable array, got type `%s`",
                thing->id,
                ti_val_str(arr));
        return -1;
    }

    n = tp == QP_ARRAY_OPEN ? -1 : (ssize_t) tp - QP_ARRAY0;

    if (n > 0)
    {
        if (vec_resize(&arr->via.arr, arr->via.arr->n + n))
        {
            log_critical(EX_ALLOC_S);
            return -1;
        }
    }
    else if (n < 0)
    {
        if (vec_resize(&arr->via.arr, arr->via.arr->n + grow_sz))
        {
            log_critical(EX_ALLOC_S);
            return -1;
        }
    }

    while(n-- && (rc = ti_val_from_unp(&val, unp, collection->things)) == 0)
    {
        ti_val_t * v;
        if (n < 0 && !vec_space(arr->via.arr))
        {
            grow_sz *= 2;
            if (vec_resize(&arr->via.arr, arr->via.arr->n + grow_sz))
            {
                log_critical(EX_ALLOC_S);
                return -1;
            }
        }

        v = ti_val_weak_dup(&val);
        if (!v)
        {
            log_critical(EX_ALLOC_S);
            return -1;
        }

        if (ti_val_move_to_arr(arr, v, e))
        {
            log_critical("job `push` to array on "TI_THING_ID": %s", e->msg);
            ti_val_destroy(v);
            return -1;
        }
    }

    if (rc < 0)
    {
        log_critical(
                "job `push` to array on "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        return -1;
    }

    /* don't care if shrink fails */
    (void) vec_shrink(&arr->via.arr);

    return 0;
}

/*
 * Returns 0 on success
 * - for example: {'attr': value}
 */
static int job__set(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ti_val_t val;
    ti_name_t * name;
    qp_obj_t qp_attr;

    if (!ti_thing_with_attrs(thing))
        return 0;

    if (!qp_is_map(qp_next(unp, NULL)) || !qp_is_raw(qp_next(unp, &qp_attr)))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "missing map or attribute",
                thing->id);
        return -1;
    }

    if (!ti_name_is_valid_strn((const char *) qp_attr.via.raw, qp_attr.len))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "invalid attribute: `%.*s`",
                thing->id,
                (int) qp_attr.len,
                (const char *) qp_attr.via.raw);
        return -1;
    }

    name = ti_names_get((const char *) qp_attr.via.raw, qp_attr.len);
    if (!name)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (ti_val_from_unp(&val, unp, collection->things))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "error reading value for attribute: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (!ti_val_is_settable(&val))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "type `%s` is not settable",
                thing->id,
                ti_val_str(&val));
        goto fail;
    }

    if (ti_thing_attr_weak_setv(thing, name, &val))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "error setting attribute: `%s` (type: `%s`)",
                thing->id,
                name->str,
                ti_val_str(&val));
        goto fail;
    }

    return 0;

fail:
    ti_val_clear(&val);
    ti_name_drop(name);
    return -1;
}

/*
 * Returns 0 on success
 * - for example: 'attr'
 */
static int job__unset(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    if (!thing->attrs)
        return 0;

    qp_obj_t qp_prop;
    ti_name_t * name;
    if (!qp_is_raw(qp_next(unp, &qp_prop)))
    {
        log_critical(
                "job `unset` attribute from "TI_THING_ID": "
                "missing attribute data",
                thing->id);
        return -1;
    }

    /* the job is already validated so getting the name will most likely
     * succeed */
    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (name)
        ti_thing_attr_unset(thing, name);

    return 0;
}
