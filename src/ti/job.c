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
static int job__rename(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__set(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp);
static int job__splice(
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
int ti_job_run(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    qp_obj_t qp_job_name;
    const uchar * raw;
    if (!qp_is_raw(qp_next(unp, &qp_job_name)) || qp_job_name.len < 3)
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
    case 'r':
        return job__rename(collection, thing, unp);
    case 's':
        return *(raw+1) == 'e'
                ? job__set(collection, thing, unp)
                : job__splice(collection, thing, unp);
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

    while(n-- && (rc = ti_val_from_unp(&val, unp, collection->things)) == 0)
    {
        ti_val_t * v;
        if (n < 0 && !vec_space(arr->via.arr))
        {
            if (vec_resize(&arr->via.arr, arr->via.arr->n + grow_sz))
            {
                log_critical(EX_ALLOC_S);
                return -1;
            }
            grow_sz *= 2;
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
 * - for example: {'from': 'to'}
 */
static int job__rename(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    qp_obj_t qp_from, qp_to;
    ti_name_t * from_name, * to_name;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_from)) ||
        !qp_is_raw(qp_next(unp, &qp_to)))
    {
        log_critical(
                "job `rename` property on "TI_THING_ID": "
                "missing map or names",
                thing->id);
        return -1;
    }

    from_name = ti_names_weak_get((const char *) qp_from.via.raw, qp_from.len);
    if (!from_name)
    {
        log_critical(
                "job `rename` property on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_from.len, (char *) qp_from.via.raw);
        return -1;
    }

    to_name = ti_names_get((const char *) qp_to.via.raw, qp_to.len);
    if (!to_name)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    if (!ti_thing_rename(thing, from_name, to_name))
    {
        log_critical(
                "job `rename` property on "TI_THING_ID": "
                "missing property: `%s`",
                thing->id,
                from_name->str);
        return -1;
    }

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
 * - for example: {'prop': [index, del_count, new_count, values...]}
 */
static int job__splice(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);
    assert (collection);
    assert (thing);
    assert (unp);

    ex_t * e = ex_use();
    int rc = 0;
    ssize_t n, i, c, cur_n, new_n;
    ti_val_t val, * arr;
    ti_name_t * name;
    qp_types_t tp;
    qp_obj_t qp_prop, qp_i, qp_c, qp_n;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_prop)) ||
        !qp_is_array((tp = qp_next(unp, NULL))) ||
        !qp_is_int(qp_next(unp, &qp_i)) ||
        !qp_is_int(qp_next(unp, &qp_c)) ||
        !qp_is_int(qp_next(unp, &qp_n)))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "missing map, property, index, delete_count or new_count",
                thing->id);
        return -1;
    }

    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !(arr = ti_thing_get(thing, name)))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_prop.len, (char *) qp_prop.via.raw);
        return -1;
    }

    if (!ti_val_is_mutable_arr(arr))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "expecting a mutable array, got type `%s`",
                thing->id,
                ti_val_str(arr));
        return -1;
    }

    cur_n = arr->via.arr->n;
    i = qp_i.via.int64;
    c = qp_c.via.int64;
    n = qp_n.via.int64;

    if (i < 0 ||
        i > cur_n ||
        c < 0 ||
        i + c > cur_n ||
        n < 0 ||
        (tp != QP_ARRAY_OPEN && n > tp - QP_ARRAY0 - 2))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "incorrect values "
                "(index: %zd, delete: %zd, new: %zd, current_size: %zd)",
                thing->id,
                i, c, n, cur_n);
        return -1;
    }

    new_n = cur_n + n - c;

    if (new_n > cur_n && vec_resize(&arr->via.arr, new_n))
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    for (ssize_t x = i, y = i + c; x < y; ++x)
        ti_val_destroy(vec_get(arr->via.arr, x));

    memmove(
        arr->via.arr->data + i + n,
        arr->via.arr->data + i + c,
        (cur_n - i - c) * sizeof(void*));

    arr->via.arr->n = i;

    while(n-- && (rc = ti_val_from_unp(&val, unp, collection->things)) == 0)
    {
        assert (vec_space(arr->via.arr));
        ti_val_t * v;

        v = ti_val_weak_dup(&val);
        if (!v)
        {
            log_critical(EX_ALLOC_S);
            return -1;
        }

        if (ti_val_move_to_arr(arr, v, e))
        {
            log_critical("job `splice` array on "TI_THING_ID": %s", e->msg);
            ti_val_destroy(v);
            return -1;
        }
    }

    if (rc)  /* both <0 and >0 are not correct since we should have n values */
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        return -1;
    }

    arr->via.arr->n = new_n;

    if (new_n < cur_n)
        (void) vec_shrink(&arr->via.arr);

    return 0;
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
