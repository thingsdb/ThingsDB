/*
 * props.h
 */
#include <props.h>
#include <stdlib.h>
#include <string.h>
#include <util/vec.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/smap.h>


static smap_t * props;
static int thingsdb__props_write_cb(ti_prop_t * prop, FILE * f);

int thingsdb_props_create(void)
{
    props = thingsdb_get()->props = smap_create();
    return -(props == NULL);
}

void thingsdb_props_destroy(void)
{
    smap_destroy(props, free);
    props = thingsdb_get()->props = NULL;
}

ti_prop_t * thingsdb_props_get(const char * name)
{
    ti_prop_t * prop = smap_get(props, name);
    if (!prop)
    {
        prop = ti_prop_create(name);
        if (!prop || smap_add(props, name, prop))
        {
            ti_prop_destroy(prop);
            return NULL;
        }
    }
    return prop;
}

int thingsdb_props_store(const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f)
        return -1;

    rc = smap_values(props, (smap_val_cb) thingsdb__props_write_cb, f);
    if (rc)
        log_error("saving failed: `%s`", fn);
    return -(fclose(f) || rc);
}

imap_t * thingsdb_props_restore(const char * fn)
{
    ssize_t sz;
    unsigned char * data = fx_read(fn, &sz);
    imap_t * propsmap = imap_create();
    if (!data || !propsmap)
        goto failed;

    unsigned char * pt = data;
    unsigned char * end = data + sz - (sizeof(intptr_t) + 2);

    while (pt < end)
    {
        ti_prop_t * prop;
        intptr_t p;

        memcpy(&p, pt, sizeof(intptr_t));
        pt += sizeof(intptr_t);
        prop = thingsdb_props_get((const char *) pt);
        if (!prop || imap_add(propsmap, (uint64_t) p, prop))
            goto failed;
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

static int thingsdb__props_write_cb(ti_prop_t * prop, FILE * f)
{
    intptr_t p = (intptr_t) prop;
    size_t sz = strlen(prop->name) + 1;
    return !(
        fwrite(&p, sizeof(intptr_t), 1, f) == 1 &&
        fwrite(prop->name, sizeof(char), sz, f) == sz
    );
}
