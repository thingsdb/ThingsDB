/*
 * ti/job.c
 */
#include <assert.h>
#include <ti.h>
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

    ti_val_t * val;
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

    val = ti_val_from_unp(unp, collection->things);
    if (!val)
    {
        log_critical(
                "job `assign` to "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (ti_thing_prop_set(thing, name, val))
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
    ti_val_drop(val);
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

    ti_val_t * val;
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

    val = ti_val_from_unp(unp, collection->things);
    if (!val)
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "error reading value for attribute: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (!ti_val_is_settable(val))
    {
        log_critical(
                "job `set` attribute on "TI_THING_ID": "
                "type `%s` is not settable",
                thing->id,
                ti_val_str(val));
        goto fail;
    }

    if (ti_thing_attr_set(thing, name, val))
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
    ti_val_drop(val);
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
    ssize_t n, i, c, cur_n, new_n;
    ti_val_t * val;
    ti_varr_t * varr;
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
                "job `splice` on "TI_THING_ID": "
                "missing map, property, index, delete_count or new_count",
                thing->id);
        return -1;
    }

    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !(varr = (ti_varr_t *) ti_thing_prop_weak_get(thing, name)))
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_prop.len, (char *) qp_prop.via.raw);
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
                "job `splice` on "TI_THING_ID": "
                "incorrect values "
                "(index: %zd, delete: %zd, new: %zd, current_size: %zd)",
                thing->id,
                i, c, n, cur_n);
        return -1;
    }

    new_n = cur_n + n - c;

    if (new_n > cur_n && vec_resize(&varr->vec, new_n))
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    for (ssize_t x = i, y = i + c; x < y; ++x)
        ti_val_destroy(vec_get(varr->vec, x));

    memmove(
            varr->vec->data + i + n,
            varr->vec->data  + i + c,
            (cur_n - i - c) * sizeof(void*));

    varr->vec->n = i;

    while(n-- && (val = ti_val_from_unp(unp, collection->things)))
    {
        if (ti_varr_append(varr, val, e))
        {
            log_critical("job `splice` array on "TI_THING_ID": %s", e->msg);
            ti_val_drop(val);
            return -1;
        }
    }

    if (!val)  /* both <0 and >0 are not correct since we should have n values */
    {
        log_critical(
                "job `splice` array on "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        return -1;
    }

    varr->vec->n = new_n;

    if (new_n < cur_n)
        (void) vec_shrink(&varr->vec);

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
