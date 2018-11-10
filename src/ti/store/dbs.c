/*
 * ti/store/dbs.c
 */
#include <util/qpx.h>
#include <ti/db.h>
#include <ti.h>
#include <ti/store/dbs.h>
#include <util/fx.h>
#include <util/vec.h>
#include <stdlib.h>
#include <string.h>


static const int ti__dbs_fn_schema = 0;

int ti_store_dbs_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(1024, 4);
    if (!packer)
        return -1;

    if (qp_add_map(&packer))
        goto stop;

    /* schema */
    if (qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, ti__dbs_fn_schema)) goto stop;

    if (qp_add_raw_from_str(packer, "dbs") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(ti()->dbs, ti_db_t, db))
    {
        if (    qp_add_array(&packer) ||
                qp_add_raw(
                        packer,
                        (const uchar *) db->guid.guid,
                        sizeof(guid_t)) ||
                qp_add_raw(packer, db->name->data, db->name->n) ||
                qp_add_array(&packer) ||
                qp_add_int64(packer, db->quota->max_things) ||
                qp_add_int64(packer, db->quota->max_props) ||
                qp_add_int64(packer, db->quota->max_array_size) ||
                qp_add_int64(packer, db->quota->max_raw_size) ||
                qp_close_array(packer) ||
                qp_close_array(packer))
            goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer))
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    qp_packer_destroy(packer);
    return rc;
}

int ti_store_dbs_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    uchar * data = fx_read(fn, &n);
    if (!data) return -1;

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);

    qp_res_t * res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    qp_res_t * schema, * qdbs;

    if (    res->tp != QP_RES_MAP ||
            !(schema = qpx_map_get(res->via.map, "schema")) ||
            !(qdbs = qpx_map_get(res->via.map, "dbs")) ||
            schema->tp != QP_RES_INT64 ||
            schema->via.int64 != ti__dbs_fn_schema ||
            qdbs->tp != QP_RES_ARRAY)
        goto stop;

    for (uint32_t i = 0; i < qdbs->via.array->n; i++)
    {
        guid_t guid;
        ti_db_t * db;
        qp_res_t * qdb = qdbs->via.array->values + i;
        qp_res_t
            * qguid, * qname, * q_quota,
            * qq_things, * qq_props, * qq_arrsz, * qq_rawsz;
        char * name;
        size_t n;
        if (    qdb->tp != QP_RES_ARRAY ||
                qdb->via.array->n != 3 ||
                !(qguid = qdb->via.array->values) ||
                !(qname = qdb->via.array->values + 1) ||
                !(q_quota = qdb->via.array->values + 2) ||
                qguid->tp != QP_RES_RAW ||
                qguid->via.raw->n != sizeof(guid_t) ||
                qname->tp != QP_RES_RAW ||
                q_quota->tp != QP_RES_ARRAY ||
                q_quota->via.array->n != 4 ||
                !(qq_things = q_quota->via.array->values) ||
                !(qq_props = q_quota->via.array->values + 1) ||
                !(qq_arrsz = q_quota->via.array->values + 2) ||
                !(qq_rawsz = q_quota->via.array->values + 3) ||
                qq_things->tp != QP_RES_INT64 ||
                qq_props->tp != QP_RES_INT64 ||
                qq_arrsz->tp != QP_RES_INT64 ||
                qq_rawsz->tp != QP_RES_INT64)
            goto stop;

        /* copy and check guid, must be null terminated */
        memcpy(guid.guid, qguid->via.raw->data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto stop;

        name = (char *) qname->via.raw->data;
        n = qname->via.raw->n;

        db = ti_db_create(&guid, name, n);
        if (!db || vec_push(&ti()->dbs, db))
            goto stop;

        db->quota->max_things = qq_things->via.int64;
        db->quota->max_props = qq_props->via.int64;
        db->quota->max_array_size = qq_arrsz->via.int64;
        db->quota->max_raw_size = qq_rawsz->via.int64;
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
