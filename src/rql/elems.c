/*
 * elems.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/elems.h>
#include <rql/item.h>
#include <util/fx.h>
#include <util/logger.h>

static void rql__elems_gc_mark(rql_elem_t * elem);

rql_elem_t * rql_elems_create(imap_t * elems, uint64_t id)
{
    rql_elem_t * elem = rql_elem_create(id);
    if (!elem || imap_add(elems, id, elem))
    {
        rql_elem_drop(elem);
        return NULL;
    }
    return elem;
}

int rql_elems_gc(imap_t * elems, rql_elem_t * root)
{
    size_t n = 0;
    vec_t * elems_vec = imap_vec_pop(elems);
    if (!elems_vec) return -1;

    if (root)
    {
        rql__elems_gc_mark(root);
    }

    for (vec_each(elems_vec, rql_elem_t, elem))
    {
        if (elem->flags & RQL_ELEM_FLAG_SWEEP)
        {
            vec_destroy(elem->items, (vec_destroy_cb) rql_item_destroy);
        }
    }

    for (vec_each(elems_vec, rql_elem_t, elem))
    {
        if (elem->flags & RQL_ELEM_FLAG_SWEEP)
        {
            ++n;
            imap_pop(elems, elem->id);
            free(elem);
            continue;
        }
        elem->flags |= RQL_ELEM_FLAG_SWEEP;
    }

    free(elems_vec);

    log_debug("garbage collection cleaned: %zd element(s)", n);

    return 0;
}

int rql_elems_store(imap_t * elems, const char * fn)
{
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    vec_t * elems_vec = imap_vec(elems);
    if (!elems_vec) goto stop;

    for (vec_each(elems_vec, rql_elem_t, elem))
    {
        if (fwrite(&elem->id, sizeof(uint64_t), 1, f) != 1) goto stop;
    }
    rc = 0;

stop:
    if (rc) log_error("saving failed: %s", fn);
    return -(fclose(f) || rc);
}

int rql_elems_store_skeleton(imap_t * elems, const char * fn)
{
    intptr_t p;
    _Bool found;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    vec_t * elems_vec = imap_vec(elems);
    if (!elems_vec) goto stop;

    if (qp_fadd_type(f, QP_MAP_OPEN)) goto stop;

    for (vec_each(elems_vec, rql_elem_t, elem))
    {
        found = 0;
        for (vec_each(elem->items, rql_item_t, item))
        {
            if (item->val.tp < RQL_VAL_ELEM) continue;
            if (!found && (
                    qp_fadd_int64(f, (int64_t) elem->id) ||
                    qp_fadd_type(f, QP_MAP_OPEN))) goto stop;

            found = 1;
            p = (intptr_t) item->prop;

            if (qp_fadd_int64(f, (int64_t) p) ||
                rql_val_to_file(&item->val, f)) goto stop;

        }
        if (found && qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;
    }

    if (qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;

    rc = 0;
stop:
    return -(fclose(f) || rc);
}

int rql_elems_store_data(imap_t * elems, const char * fn)
{
    intptr_t p;
    _Bool found;
    int rc = -1;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    vec_t * elems_vec = imap_vec(elems);
    if (!elems_vec) goto stop;

    if (qp_fadd_type(f, QP_MAP_OPEN)) goto stop;

    for (vec_each(elems_vec, rql_elem_t, elem))
    {
        found = 0;
        for (vec_each(elem->items, rql_item_t, item))
        {
            if (item->val.tp >= RQL_VAL_ELEM) continue;
            if (!found && (
                    qp_fadd_int64(f, (int64_t) elem->id) ||
                    qp_fadd_type(f, QP_MAP_OPEN))) goto stop;

            found = 1;
            p = (intptr_t) item->prop;

            if (qp_fadd_int64(f, (int64_t) p) ||
                rql_val_to_file(&item->val, f)) goto stop;
        }
        if (found && qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;
    }

    if (qp_fadd_type(f, QP_MAP_CLOSE)) goto stop;

    rc = 0;
stop:
    return -(fclose(f) || rc);
}

int rql_elems_restore(imap_t * elems, const char * fn)
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
        if (!rql_elems_create(elems, id)) goto failed;
    }

    goto done;

failed:
    rc = -1;
    log_critical("failed to restore from file: '%s'", fn);
done:
    free(data);
    return rc;
}

int rql_elems_restore_skeleton(imap_t * elems, imap_t * props, const char * fn)
{
    int rc = 0;
    qp_types_t tp;
    rql_elem_t * elem, * el;
    rql_prop_t * prop;
    qp_res_t elem_id, prop_id;
    vec_t * elems_vec = NULL;
    FILE * f = fopen(fn, "r");

    if (!f)
    {
        log_critical("file is missing: '%s'", fn);
        return -1;
    }

    if (!qp_is_map(qp_fnext(f, NULL))) goto failed;

    while (qp_is_int(qp_fnext(f, &elem_id)))
    {
        elem = imap_get(elems, (uint64_t) elem_id.via.int64);
        if (!elem)
        {
            log_critical("cannot find element with id: %"PRId64,
                    elem_id.via.int64);
            goto failed;
        }
        if (!qp_is_map(qp_fnext(f, NULL))) goto failed;
        while (qp_is_int(qp_fnext(f, &prop_id)))
        {
            prop = imap_get(props, (uint64_t) prop_id.via.int64);
            if (!prop)
            {
                log_critical("cannot find prop with id: %"PRId64,
                        prop_id.via.int64);
                goto failed;
            }
            tp = qp_fnext(f, &elem_id);
            if (qp_is_int(tp))
            {
                el = imap_get(elems, (uint64_t) elem_id.via.int64);
                if (!el)
                {
                    log_critical("cannot find element with id: %"PRId64,
                            elem_id.via.int64);
                    goto failed;
                }
                if (rql_elem_set(elem, prop, RQL_VAL_ELEM, el)) goto failed;
            }
            else if (qp_is_array(tp))
            {
                elems_vec = vec_new(0);
                while (qp_is_int(qp_fnext(f, &elem_id)))
                {
                    el = imap_get(elems, (uint64_t) elem_id.via.int64);
                    if (!el)
                    {
                        log_critical("cannot find element with id: %"PRId64,
                                elem_id.via.int64);
                        goto failed;
                    }
                    if (vec_push(&elems_vec, rql_elem_grab(el)))
                    {
                        rql_elem_drop(el);
                        goto failed;
                    }
                }
                if (rql_elem_weak_set(elem, prop, RQL_VAL_ELEMS, elems_vec))
                {
                    goto failed;
                }
                elems_vec = NULL;
            }
            else
            {
                log_critical("unexpected type: %d", elem_id.tp);
                goto failed;
            }
        }
    }
    goto done;

failed:
    rc = -1;
    vec_destroy(elems_vec, (vec_destroy_cb) rql_elem_drop);
    log_critical("failed to restore from file: '%s'", fn);
done:
    fclose(f);
    return rc;
}

int rql_elems_restore_data(imap_t * elems, imap_t * props, const char * fn)
{
    int rc = 0;
    qp_types_t tp;
    rql_elem_t * elem;
    rql_prop_t * prop;
    qp_res_t elem_id, prop_id, obj;
    vec_t * elems_vec = NULL;
    FILE * f = fopen(fn, "r");

    if (!f)
    {
        log_critical("file is missing: '%s'", fn);
        return -1;
    }

    if (!qp_is_map(qp_fnext(f, NULL))) goto failed;

    while (qp_is_int(qp_fnext(f, &elem_id)))
    {
        elem = imap_get(elems, (uint64_t) elem_id.via.int64);
        if (!elem)
        {
            log_critical("cannot find element with id: %"PRId64,
                    elem_id.via.int64);
            goto failed;
        }
        if (!qp_is_map(qp_fnext(f, NULL))) goto failed;
        while (qp_is_int(qp_fnext(f, &prop_id)))
        {
            prop = imap_get(props, (uint64_t) prop_id.via.int64);
            if (!prop)
            {
                log_critical("cannot find prop with id: %"PRId64,
                        prop_id.via.int64);
                goto failed;
            }
            tp = qp_fnext(f, &obj);
            switch (tp)
            {
            case QP_INT64:
                if (rql_elem_set(elem, prop, RQL_VAL_INT, &obj.via.int64))
                {
                    goto failed;
                }
                break;
            case QP_RAW:
                if (!obj.via.raw) goto failed;
                if (rql_elem_weak_set(elem, prop, RQL_VAL_RAW, obj.via.raw))
                {
                    goto failed;
                }
                break;
            case QP_NULL:
                if (rql_elem_set(elem, prop, RQL_VAL_NIL, NULL))
                {
                    goto failed;
                }
                break;
            case QP_TRUE:
            case QP_FALSE:
                if (rql_elem_set(elem, prop, RQL_VAL_BOOL, &obj.via.boolean))
                {
                    goto failed;
                }
                break;
            case QP_MAP_CLOSE:
                break;
            default:
                log_critical("unexpected type: %d", tp);
                goto failed;
            }
        }
    }
    goto done;

failed:
    rc = -1;
    vec_destroy(elems_vec, (vec_destroy_cb) rql_elem_drop);
    log_critical("failed to restore from file: '%s'", fn);
done:
    fclose(f);
    return rc;
}

static void rql__elems_gc_mark(rql_elem_t * elem)
{
    elem->flags &= ~RQL_ELEM_FLAG_SWEEP;
    for (vec_each(elem->items, rql_item_t, item))
    {
        switch (item->val.tp)
        {
        case RQL_VAL_ELEM:
            if (item->val.via.elem_->flags & RQL_ELEM_FLAG_SWEEP)
            {
                rql__elems_gc_mark(item->val.via.elem_);
            }
            continue;
        case RQL_VAL_ELEMS:
            for (vec_each(item->val.via.elems_, rql_elem_t, el))
            {
                if (el->flags & RQL_ELEM_FLAG_SWEEP)
                {
                    rql__elems_gc_mark(el);
                }
            }
            continue;
        default:
            continue;
        }
    }
}


