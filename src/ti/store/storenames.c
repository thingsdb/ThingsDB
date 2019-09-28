/*
 * ti/store/names.c
 */
#include <qpack.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/store/storenames.h>
#include <util/fx.h>
#include <util/logger.h>
#include <util/smap.h>

static int names__write_cb(ti_name_t * name, FILE * f);


int ti_store_names_store(const char * fn)
{
    int rc;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_error("cannot open file `%s` (%s)", fn, strerror(errno));
        return -1;
    }

    rc = (
        qp_fadd_type(f, QP_ARRAY_OPEN) ||
        smap_values(ti()->names, (smap_val_cb) names__write_cb, f)
    );

    if (rc)
        log_error("error writing to file `%s`", fn);

    if (fclose(f))
    {
        log_error("cannot close file `%s` (%s)", fn, strerror(errno));
        rc = -1;
    }

    if (rc == 0)
        log_debug("stored names to file: `%s`", fn);

    return rc;
}

imap_t * ti_store_names_restore(const char * fn)
{
    ssize_t sz;
    qp_unpacker_t unpacker;
    qp_obj_t qpiptr, qpname;
    uchar * data = fx_read(fn, &sz);
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

static int names__write_cb(ti_name_t * name, FILE * f)
{
    intptr_t p = (intptr_t) name;
    return name->str[0] == '$' ? 0 : (
        qp_fadd_type(f, QP_ARRAY2) ||
        qp_fadd_int(f, p) ||
        qp_fadd_raw(f, (const uchar *) name->str, name->n)
    );
}
