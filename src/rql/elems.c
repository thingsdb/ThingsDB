/*
 * elems.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <rql/elems.h>
#include <rql/item.h>
#include <util/logger.h>

static int rql__elems_write_cb(rql_elem_t * elem, FILE * f);
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
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    rc = imap_walk(elems, (imap_cb) rql__elems_write_cb, f);

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

    for (;pt < end; pt += sizeof(uint64_t))
    {
        uint64_t id;
        LOGC("Reading ID: %"PRIu64, id);
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

static int rql__elems_write_cb(rql_elem_t * elem, FILE * f)
{
    return fwrite(&elem->id, sizeof(uint64_t), 1, f) == 1;
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


