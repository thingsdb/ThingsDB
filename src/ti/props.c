/*
 * props.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti/props.h>
#include <ti.h>
#include <util/vec.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/smap.h>

static smap_t * props;
static int ti__props_write_cb(ti_prop_t * prop, FILE * f);

int ti_props_create(void)
{
    props = ti_get()->props = smap_create();
    return -(props == NULL);
}

void ti_props_destroy(void)
{
    if (!props)
        return;
    smap_destroy(props, free);
    props = ti_get()->props = NULL;
}

ti_prop_t * ti_props_get(const char * name, size_t n)
{
    ti_prop_t * prop = smap_getn(props, name, n);
    if (!prop)
    {
        prop = ti_prop_create(name, n);
        if (!prop || smap_add(props, prop->name, prop))
        {
            ti_prop_destroy(prop);
            return NULL;
        }
    }
    return prop;
}

int ti_props_store(const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f)
        return -1;
    rc = (  qp_fadd_type(f, QP_ARRAY_OPEN) ||
            smap_values(props, (smap_val_cb) ti__props_write_cb, f));
    if (rc)
        log_error("saving failed: `%s`", fn);
    return -(fclose(f) || rc);
}

imap_t * ti_props_restore(const char * fn)
{
    ssize_t sz;
    qp_unpacker_t unpacker;
    qp_obj_t qpiptr, qpname;
    unsigned char * data = fx_read(fn, &sz);
    imap_t * propsmap = imap_create();
    if (!data || !propsmap)
        goto failed;

    qp_unpacker_init2(&unpacker, data, (size_t) sz, 0);
    if (!qp_is_array(qp_next(&unpacker, NULL)))
        goto failed;

    while (qp_is_array(qp_next(&unpacker, NULL)))
    {
        ti_prop_t * prop;
        if (    !qp_is_int(qp_next(&unpacker, &qpiptr)) ||
                !qp_is_raw(qp_next(&unpacker, &qpname)))
            goto failed;

        prop = ti_props_get(qpname.via.raw, qpname.len);
        if (!prop || imap_add(propsmap, (uint64_t) qpiptr.via.int64, prop))
            goto failed;
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

static int ti__props_write_cb(ti_prop_t * prop, FILE * f)
{
    intptr_t p = (intptr_t) prop;
    return (qp_fadd_type(f, QP_ARRAY2) ||
            qp_fadd_int64(f, p) ||
            qp_fadd_raw(f, (const unsigned char *) prop->name, prop->n));
}
