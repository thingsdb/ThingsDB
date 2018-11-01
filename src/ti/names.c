/*
 * names.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti/names.h>
#include <ti.h>
#include <util/vec.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/smap.h>

static smap_t * names;
static int ti__names_write_cb(ti_name_t * name, FILE * f);

int ti_names_create(void)
{
    names = ti()->names = smap_create();
    return -(names == NULL);
}

void ti_names_destroy(void)
{
    if (!names)
        return;
    smap_destroy(names, free);
    names = ti()->names = NULL;
}

/*
 * returns a name with a new reference or NULL in case of an error
 */
ti_name_t * ti_names_get(const char * str, size_t n)
{
    ti_name_t * name = smap_getn(names, str, n);
    if (name)
        return ti_grab(name);

    name = ti_name_create(str, n);
    if (!name || smap_add(names, name->str, name))
    {
        ti_name_destroy(name);
        return NULL;
    }
    return name;
}

/*
 * returns a name when the name exists and with a borrowed reference, if the
 * name does not exists, NULL will be the return value.
 */
ti_name_t * ti_names_weak_get(const char * str, size_t n)
{
    return smap_getn(names, str, n);
}

int ti_names_store(const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f)
        return -1;
    rc = (  qp_fadd_type(f, QP_ARRAY_OPEN) ||
            smap_values(names, (smap_val_cb) ti__names_write_cb, f));
    if (rc)
        log_error("saving failed: `%s`", fn);
    return -(fclose(f) || rc);
}

imap_t * ti_names_restore(const char * fn)
{
    ssize_t sz;
    qp_unpacker_t unpacker;
    qp_obj_t qpiptr, qpname;
    unsigned char * data = fx_read(fn, &sz);
    imap_t * namesmap = imap_create();
    if (!data || !namesmap)
        goto failed;

    qp_unpacker_init2(&unpacker, data, (size_t) sz, 0);
    if (!qp_is_array(qp_next(&unpacker, NULL)))
        goto failed;

    while (qp_is_array(qp_next(&unpacker, NULL)))
    {
        ti_name_t * name;
        if (    !qp_is_int(qp_next(&unpacker, &qpiptr)) ||
                !qp_is_raw(qp_next(&unpacker, &qpname)))
            goto failed;

        name = ti_names_get((char *) qpname.via.raw, qpname.len);
        if (!name || imap_add(namesmap, (uint64_t) qpiptr.via.int64, name))
            goto failed;
    }

    goto done;

failed:
    log_critical("failed to restore from file: `%s`", fn);
    imap_destroy(namesmap, NULL);
    namesmap = NULL;
done:
    free(data);
    return namesmap;
}

static int ti__names_write_cb(ti_name_t * name, FILE * f)
{
    intptr_t p = (intptr_t) name;
    return (qp_fadd_type(f, QP_ARRAY2) ||
            qp_fadd_int64(f, p) ||
            qp_fadd_raw(f, (const unsigned char *) name->str, name->n));
}
