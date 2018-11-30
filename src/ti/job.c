/*
 * ti/job.c
 */
#include <assert.h>
#include <ti/job.h>
#include <ti/val.h>
#include <ti/name.h>
#include <ti/names.h>

static int job__assign(ti_db_t * db, ti_thing_t * thing, qp_unpacker_t * unp);

/*
 * (Log function)
 * Unpacker should be at point 'job': ...
 */
int ti_job_run(ti_db_t * db, ti_thing_t * thing, qp_unpacker_t * unp)
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
        return job__assign(db, thing, unp);
    }

    log_critical("unknown job: `%.*s`", (int) qp_job_name.len, (char *) raw);
    return -1;
}


static int job__assign(ti_db_t * db, ti_thing_t * thing, qp_unpacker_t * unp)
{
    assert (db);

    ti_val_t * val;
    ti_name_t * name;
    qp_obj_t qp_prop;
    if (!qp_is_map(qp_next(unp, NULL)) || !qp_is_raw(qp_next(unp, &qp_prop)))
    {
        log_critical(
                "job `assign` on "TI_THING_ID": "
                "missing map or property",
                thing->id);
        return -1;
    }

    if (!ti_name_is_valid_strn((const char *) qp_prop.via.raw, qp_prop.len))
    {
        log_critical(
                "job `assign` on "TI_THING_ID": "
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

    val = ti_val_from_unp(unp, db->things);
    if (!val)
    {
        log_critical("job `assign` on "TI_THING_ID": "
                "error reading value for property: `%s`",
                thing->id,
                name->str);
        goto fail;
    }

    if (ti_thing_weak_setv(thing, name, val))
    {
        log_critical(
                "job `assign` on "TI_THING_ID": "
                "error setting property: `%s` (type: `%s`)",
                thing->id,
                name->str,
                ti_val_str(val));
        goto fail;
    }

    return 0;

fail:
    ti_val_destroy(val);
    ti_name_drop(name);
    return -1;
}
