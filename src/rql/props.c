/*
 * props.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <rql/props.h>
#include <util/vec.h>
#include <util/fx.h>
#include <util/logger.h>

static int rql__props_gc_cb(rql_prop_t * prop, vec_t ** propsgc);
static int rql__props_write_cb(rql_prop_t * prop, FILE * f);

rql_prop_t * rql_db_props_get(smap_t * props, const char * name)
{
    rql_prop_t * prop = (rql_prop_t *) smap_get(props, name);
    if (!prop)
    {
        prop = rql_prop_create(name);
        if (!prop || smap_add(props, name, prop))
        {
            rql_prop_drop(prop);
            return NULL;
        }
    }
    return prop;
}

int rql_props_gc(smap_t * props)
{
    int rc = -1;
    vec_t * propsgc = vec_new(0);
    if (!propsgc ||
        smap_values(props, (smap_val_cb) rql__props_gc_cb, &propsgc)) goto stop;

    for (vec_each(propsgc, rql_prop_t, prop))
    {
        smap_pop(props, prop->name);
        rql_prop_drop(prop);
    }

    rc = 0;
    log_debug("garbage collection cleaned: %lu properties(s)", propsgc->n);

stop:
    free(propsgc);
    return rc;
}

int rql_props_store(smap_t * props, const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f) return -1;

    rc = smap_values(props, (smap_val_cb) rql__props_write_cb, f);
    if (rc) log_error("saving failed: `%s`", fn);
    return -(fclose(f) || rc);
}

imap_t * rql_props_restore(smap_t * props, const char * fn)
{
    ssize_t sz;
    unsigned char * data = fx_read(fn, &sz);
    imap_t * propsmap = imap_create();
    if (!data || !propsmap) goto failed;

    unsigned char * pt = data;
    unsigned char * end = data + sz - (sizeof(intptr_t) + 2);

    while (pt < end)
    {
        rql_prop_t * prop;
        intptr_t p;

        memcpy(&p, pt, sizeof(intptr_t));
        pt += sizeof(intptr_t);
        prop = rql_db_props_get(props, (const char *) pt);
        if (!prop || imap_add(propsmap, (uint64_t) p, prop)) goto failed;
        pt += strlen(prop->name) + 1;
    }

    goto done;

failed:
    log_critical("failed to restore from file: `%s`", fn);
    imap_destroy(propsmap, NULL);
    propsmap = NULL;
done:
    free(data);
    return propsmap;
}

static int rql__props_gc_cb(rql_prop_t * prop, vec_t ** propsgc)
{
    return (prop->ref == 1) ? vec_push(propsgc, prop) : 0;
}

static int rql__props_write_cb(rql_prop_t * prop, FILE * f)
{
    intptr_t p = (intptr_t) prop;
    size_t sz = strlen(prop->name) + 1;
    return !(
        fwrite(&p, sizeof(intptr_t), 1, f) == 1 &&
        fwrite(prop->name, sizeof(char), sz, f) == sz);
}
