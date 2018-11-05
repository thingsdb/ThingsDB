/*
 * ti/things.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/things.h>
#include <util/fx.h>
#include <util/logger.h>
#include <ti/prop.h>
#include <ti.h>

static void things__gc_mark(ti_thing_t * thing);

ti_thing_t * ti_things_create_thing(imap_t * things, uint64_t id)
{
    assert (id);
    ti_thing_t * thing = ti_thing_create(id, things);
    if (!thing || ti_thing_to_map(thing))
    {
        ti_thing_drop(thing);
        return NULL;
    }
    return thing;
}

int ti_things_gc(imap_t * things, ti_thing_t * root)
{
    size_t n = 0;
    vec_t * things_vec = imap_vec_pop(things);
    if (!things_vec)
        return -1;

    if (root)
    {
        things__gc_mark(root);
    }

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (thing->flags & TI_THING_FLAG_SWEEP)
        {
            thing->ref = 0;
        }
    }

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (thing->flags & TI_THING_FLAG_SWEEP)
        {
            ++n;
            vec_destroy(thing->props, (vec_destroy_cb) ti_prop_destroy);
            (void *) imap_pop(things, thing->id);
            free(thing);
            continue;
        }
        thing->flags |= TI_THING_FLAG_SWEEP;
    }

    free(things_vec);

    log_debug("garbage collection cleaned: %zd thing(s)", n);

    return 0;
}

int ti_things_store(imap_t * things, const char * fn)
{
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f)
        return -1;

    vec_t * things_vec = imap_vec(things);
    if (!things_vec) goto stop;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        if (fwrite(&thing->id, sizeof(uint64_t), 1, f) != 1)
            goto stop;
    }
    rc = 0;

stop:
    if (rc)
        log_error("save failed: %s", fn);
    return -(fclose(f) || rc);
}

int ti_things_store_skeleton(imap_t * things, const char * fn)
{
    intptr_t p;
    _Bool found;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    vec_t * things_vec = imap_vec(things);
    if (!things_vec)
        goto stop;

    if (qp_fadd_type(f, QP_MAP_OPEN)) goto stop;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        found = 0;
        for (vec_each(thing->props, ti_prop_t, prop))
        {
            if (prop->val.tp < TI_VAL_THING) continue;
            if (!found && (
                    qp_fadd_int64(f, (int64_t) thing->id) ||
                    qp_fadd_type(f, QP_MAP_OPEN))) goto stop;

            found = 1;
            p = (intptr_t) prop->name;

            if (qp_fadd_int64(f, (int64_t) p) ||
                ti_val_to_file(&prop->val, f)) goto stop;

        }
        if (found && qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;
    }

    if (qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;

    rc = 0;
stop:
    return -(fclose(f) || rc);
}

int ti_things_store_data(imap_t * things, const char * fn)
{
    intptr_t p;
    _Bool found;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    vec_t * things_vec = imap_vec(things);
    if (!things_vec) goto stop;

    if (qp_fadd_type(f, QP_MAP_OPEN)) goto stop;

    for (vec_each(things_vec, ti_thing_t, thing))
    {
        found = 0;
        for (vec_each(thing->props, ti_prop_t, prop))
        {
            if (prop->val.tp >= TI_VAL_THING) continue;
            if (!found && (
                    qp_fadd_int64(f, (int64_t) thing->id) ||
                    qp_fadd_type(f, QP_MAP_OPEN))) goto stop;

            found = 1;
            p = (intptr_t) prop->name;

            if (qp_fadd_int64(f, (int64_t) p) ||
                ti_val_to_file(&prop->val, f)) goto stop;
        }
        if (found && qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;
    }

    if (qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;

    rc = 0;
stop:
    return -(fclose(f) || rc);
}

int ti_things_restore(imap_t * things, const char * fn)
{
    int rc = 0;
    ssize_t sz;
    unsigned char * data = fx_read(fn, &sz);
    if (!data) goto failed;

    unsigned char * pt = data;
    unsigned char * end = data + sz - sizeof(uint64_t);

    for (;pt <= end; pt += sizeof(uint64_t))
    {
        uint64_t id;
        memcpy(&id, pt, sizeof(uint64_t));
        if (!ti_things_create_thing(things, id))
            goto failed;
    }

    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: `%s`", fn);
done:
    free(data);
    return rc;
}

int ti_things_restore_skeleton(imap_t * things, imap_t * names, const char * fn)
{
    int rc = 0;
    qp_types_t tp;
    ti_thing_t * thing, * ting;
    ti_name_t * name;
    qp_res_t thing_id, name_id;
    vec_t * things_vec = NULL;
    FILE * f = fopen(fn, "r");

    if (!f)
    {
        log_critical("file is missing: `%s`", fn);
        return -1;
    }

    if (!qp_is_map(qp_fnext(f, NULL)))
        goto failed;

    while (qp_is_int(qp_fnext(f, &thing_id)))
    {
        thing = imap_get(things, (uint64_t) thing_id.via.int64);
        if (!thing)
        {
            log_critical("cannot find thing with id: %"PRId64,
                    thing_id.via.int64);
            goto failed;
        }
        if (!qp_is_map(qp_fnext(f, NULL)))
            goto failed;

        while (qp_is_int(qp_fnext(f, &name_id)))
        {
            name = imap_get(names, (uint64_t) name_id.via.int64);
            if (!name)
            {
                log_critical("cannot find name with id: %"PRId64,
                        name_id.via.int64);
                goto failed;
            }
            tp = qp_fnext(f, &thing_id);
            if (qp_is_int(tp))
            {
                ting = imap_get(things, (uint64_t) thing_id.via.int64);
                if (!ting)
                {
                    log_critical("cannot find thing with id: %"PRId64,
                            thing_id.via.int64);
                    goto failed;
                }
                if (ti_thing_set(thing, name, TI_VAL_THING, ting))
                    goto failed;
                ti_incref(name);
            }
            else if (qp_is_array(tp))
            {
                things_vec = vec_new(0);
                while (qp_is_int(qp_fnext(f, &thing_id)))
                {
                    ting = imap_get(things, (uint64_t) thing_id.via.int64);
                    if (!ting)
                    {
                        log_critical("cannot find thing with id: %"PRId64,
                                thing_id.via.int64);
                        goto failed;
                    }
                    ti_incref(ting);
                    if (vec_push(&things_vec, ting))
                    {
                        ti_thing_drop(ting);
                        goto failed;
                    }
                }
                if (ti_thing_weak_set(thing, name, TI_VAL_THINGS, things_vec))
                    goto failed;
                ti_incref(name);
                things_vec = NULL;
            }
            else
            {
                log_critical("unexpected type: %d", thing_id.tp);
                goto failed;
            }
        }
    }
    goto done;

failed:
    rc = -1;
    vec_destroy(things_vec, (vec_destroy_cb) ti_thing_drop);
    log_critical("failed to restore from file: `%s`", fn);
done:
    fclose(f);
    return rc;
}

int ti_things_restore_data(imap_t * things, imap_t * names, const char * fn)
{
    int rc = 0;
    qp_types_t tp;
    ti_thing_t * thing;
    ti_name_t * name;
    qp_res_t thing_id, name_id, obj;
    vec_t * things_vec = NULL;
    FILE * f = fopen(fn, "r");

    if (!f)
    {
        log_critical("file is missing: `%s`", fn);
        return -1;
    }

    if (!qp_is_map(qp_fnext(f, NULL))) goto failed;

    while (qp_is_int(qp_fnext(f, &thing_id)))
    {
        thing = imap_get(things, (uint64_t) thing_id.via.int64);
        if (!thing)
        {
            log_critical("cannot find thing with id: %"PRId64,
                    thing_id.via.int64);
            goto failed;
        }
        if (!qp_is_map(qp_fnext(f, NULL))) goto failed;
        while (qp_is_int(qp_fnext(f, &name_id)))
        {
            name = imap_get(names, (uint64_t) name_id.via.int64);
            if (!name)
            {
                log_critical("cannot find name with id: %"PRId64,
                        name_id.via.int64);
                goto failed;
            }
            tp = qp_fnext(f, &obj);
            switch (tp)
            {
            case QP_INT64:
                if (ti_thing_set(thing, name, TI_VAL_INT, &obj.via.int64))
                    goto failed;
                break;
            case QP_DOUBLE:
                if (ti_thing_set(thing, name, TI_VAL_FLOAT, &obj.via.real))
                    goto failed;
                break;
            case QP_RAW:
                if (!obj.via.raw)
                    goto failed;
                if (ti_thing_weak_set(thing, name, TI_VAL_RAW, obj.via.raw))
                    goto failed;
                break;
            case QP_NULL:
                if (ti_thing_set(thing, name, TI_VAL_NIL, NULL))
                    goto failed;
                break;
            case QP_TRUE:
            case QP_FALSE:
                if (ti_thing_set(thing, name, TI_VAL_BOOL, &obj.via.boolean))
                    goto failed;
                break;
            case QP_MAP_CLOSE:
                break;
            default:
                log_critical("unexpected type: %d", tp);
                goto failed;
            }
            ti_incref(name);
        }
    }
    goto done;

failed:
    rc = -1;
    vec_destroy(things_vec, (vec_destroy_cb) ti_thing_drop);
    log_critical("failed to restore from file: `%s`", fn);
done:
    fclose(f);
    return rc;
}

static void things__gc_mark(ti_thing_t * thing)
{
    thing->flags &= ~TI_THING_FLAG_SWEEP;
    for (vec_each(thing->props, ti_prop_t, prop))
    {
        switch (prop->val.tp)
        {
        case TI_VAL_THING:
            if (prop->val.via.thing->flags & TI_THING_FLAG_SWEEP)
            {
                things__gc_mark(prop->val.via.thing);
            }
            continue;
        case TI_VAL_THINGS:
            for (vec_each(prop->val.via.things, ti_thing_t, thing))
            {
                if (thing->flags & TI_THING_FLAG_SWEEP)
                {
                    things__gc_mark(thing);
                }
            }
            continue;
        default:
            continue;
        }
    }
}


