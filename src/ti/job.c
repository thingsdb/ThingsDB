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
#include <ti/syntax.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <ti/vset.h>

/*
 * Returns 0 on success
 * - for example: {'prop': [new_count, values...]}
 */
static int job__add(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ssize_t n;
    ti_vset_t * vset;
    ti_name_t * name;
    ti_thing_t * t;

    qp_obj_t qp_prop, qp_n;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_prop)) ||
        !qp_is_array(qp_next(unp, NULL)) ||
        !qp_is_int(qp_next(unp, &qp_n)) || qp_n.via.int64 <= 0)
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
                "missing map, property or new_count",
                thing->id);
        return -1;
    }

    n = qp_n.via.int64;

    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !(vset = (ti_vset_t *) ti_thing_val_weak_get(thing, name)))
    {
        log_critical(
                "job `add` to set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_prop.len, (char *) qp_prop.via.raw);
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

    while(n--)
    {
        t = (ti_thing_t *) ti_val_from_unp(unp, collection->things);

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
    qp_obj_t qp_prop;
    if (!qp_is_map(qp_next(unp, NULL)) || !qp_is_raw(qp_next(unp, &qp_prop)))
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "missing map or property",
                thing->id);
        return -1;
    }

    if (!ti_name_is_valid_strn((const char *) qp_prop.via.raw, qp_prop.len))
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "invalid property: `%.*s`",
                thing->id,
                (int) qp_prop.len,
                (const char *) qp_prop.via.raw);
        return -1;
    }

    name = ti_names_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name)
    {
        log_critical(EX_MEMORY_S);
        return -1;
    }

    val = ti_val_from_unp(unp, collection->things);
    if (!val)
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (!ti_thing_prop_set(thing, name, val))
    {
        log_critical(
                "job `set` to "TI_THING_ID": "
                "error setting property: `%s` (type: `%s`)",
                thing->id,
                name->str,
                ti_val_str(val));
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
static int job__del(ti_thing_t * thing, qp_unpacker_t * unp)
{
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
 * - for example: 'name'
 */
static int job__del_procedure(
        ti_collection_t * collection,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (unp);

    qp_obj_t qp_name;
    ti_procedure_t * procedure;

    if (!qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical(
                "job `del_procedure` in "TI_COLLECTION_ID": "
                "missing procedure name",
                collection->root->id);
        return -1;
    }

    procedure = ti_procedures_pop_strn(
            collection->procedures,
            (const char *) qp_name.via.raw,
            qp_name.len);

    if (!procedure)
    {
        log_critical(
                "job `del_procedure` cannot find `%.*s` in "TI_COLLECTION_ID,
                (int) qp_name.len, (const char *) qp_name.via.raw,
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
static int job__new_procedure(
        ti_collection_t * collection,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (unp);

    int rc;
    qp_obj_t qp_name;
    ti_procedure_t * procedure;
    ti_closure_t * closure;
    ti_raw_t * rname;


    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_name)))
    {
        log_critical(
                "job `new_procedure` for "TI_COLLECTION_ID": "
                "missing map or name",
                collection->root->id);
        return -1;
    }

    rname = ti_raw_create(qp_name.via.raw, qp_name.len);
    closure = (ti_closure_t *) ti_val_from_unp(unp, collection->things);
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
 * - for example: {'prop': [del_count, thing_ids...]}
 */
static int job__remove(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ssize_t n;
    ti_vset_t * vset;
    ti_name_t * name;
    qp_obj_t qp_prop, qp_i;
    ti_thing_t * t;
    uint64_t thing_id;

    if (!qp_is_map(qp_next(unp, NULL)) ||
        !qp_is_raw(qp_next(unp, &qp_prop)) ||
        !qp_is_array(qp_next(unp, NULL)) ||
        !qp_is_int(qp_next(unp, &qp_i)))
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "missing map, property or delete_count",
                thing->id);
        return -1;
    }

    name = ti_names_weak_get((const char *) qp_prop.via.raw, qp_prop.len);
    if (!name || !(vset = (ti_vset_t *) ti_thing_val_weak_get(thing, name)))
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "missing property: `%.*s`",
                thing->id,
                (int) qp_prop.len, (char *) qp_prop.via.raw);
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

    n = qp_i.via.int64;

    if (n < 0)
    {
        log_critical(
                "job `remove` from set on "TI_THING_ID": "
                "incorrect values "
                "(delete_count: %zd)",
                thing->id, n);
        return -1;
    }

    while(n--)
    {
        if (!qp_is_int(qp_next(unp, &qp_i)))
        {
            log_critical(
                    "job `remove` from set on "TI_THING_ID": "
                    "error reading integer value for property: `%s`",
                    thing->id,
                    name->str);
            return -1;
        }

        thing_id = (uint64_t) qp_i.via.int64;
        t = imap_get(collection->things, thing_id);

        if (!t || !ti_vset_pop(vset, t))
        {
            log_error(
                    "job `remove` from set on "TI_THING_ID": "
                    "missing thing: "TI_THING_ID,
                    thing->id,
                    thing_id);
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
static int job__splice(
        ti_collection_t * collection,
        ti_thing_t * thing,
        qp_unpacker_t * unp)
{
    assert (collection);
    assert (thing);
    assert (unp);

    ex_t e = {0};
    ssize_t n, i, c, cur_n, new_n;
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
    if (!name || !(varr = (ti_varr_t *) ti_thing_val_weak_get(thing, name)))
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
        ti_val_t * val = ti_val_from_unp(unp, collection->things);

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
        return job__add(collection, thing, unp);
    case 'd':
        return qp_job_name.len == 3
                ? job__del(thing, unp)
                : job__del_procedure(collection, unp);
    case 'n':
        return job__new_procedure(collection, unp);
    case 'r':
        return job__remove(collection, thing, unp);
    case 's':
        return qp_job_name.len == 3
                ? job__set(collection, thing, unp)
                : job__splice(collection, thing, unp);
    }

    log_critical("unknown job: `%.*s`", (int) qp_job_name.len, (char *) raw);
    return -1;
}
