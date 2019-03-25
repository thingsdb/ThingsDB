/*
 * ti/store/collections.c
 */
#include <util/qpx.h>
#include <ti/collection.h>
#include <ti.h>
#include <ti/store/collections.h>
#include <util/fx.h>
#include <util/vec.h>
#include <stdlib.h>
#include <string.h>


int ti_store_collections_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(1024, 4);
    if (!packer)
        return -1;

    if (qp_add_map(&packer))
        goto stop;

    if (qp_add_raw_from_str(packer, "collections") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(ti()->collections->vec, ti_collection_t, collection))
    {
        if (qp_add_array(&packer) ||
            qp_add_raw(
                    packer,
                    (const uchar *) collection->guid.guid,
                    sizeof(guid_t)) ||
            qp_add_raw(packer, collection->name->data, collection->name->n) ||
            qp_add_array(&packer) ||
            ti_quota_val_to_packer(packer, collection->quota->max_things) ||
            ti_quota_val_to_packer(packer, collection->quota->max_props) ||
            ti_quota_val_to_packer(packer, collection->quota->max_array_size) ||
            ti_quota_val_to_packer(packer, collection->quota->max_raw_size) ||
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

int ti_store_collections_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    qp_res_t * res;
    uchar * data = fx_read(fn, &n);
    qp_unpacker_t unpacker;
    qp_res_t * qcollections;
    if (!data)
        return -1;

    ti_collections_clear();

    qpx_unpacker_init(&unpacker, data, (size_t) n);

    res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    if (    res->tp != QP_RES_MAP ||
            !(qcollections = qpx_map_get(res->via.map, "collections")) ||
            qcollections->tp != QP_RES_ARRAY)
        goto stop;

    for (uint32_t i = 0; i < qcollections->via.array->n; i++)
    {
        guid_t guid;
        ti_collection_t * collection;
        qp_res_t * qcollection = qcollections->via.array->values + i;
        qp_res_t
            * qguid, * qname, * q_quota,
            * qq_things, * qq_props, * qq_arrsz, * qq_rawsz;
        char * name;
        size_t n;
        if (    qcollection->tp != QP_RES_ARRAY ||
                qcollection->via.array->n != 3 ||
                !(qguid = qcollection->via.array->values) ||
                !(qname = qcollection->via.array->values + 1) ||
                !(q_quota = qcollection->via.array->values + 2) ||
                qguid->tp != QP_RES_RAW ||
                qguid->via.raw->n != sizeof(guid_t) ||
                qname->tp != QP_RES_RAW ||
                q_quota->tp != QP_RES_ARRAY ||
                q_quota->via.array->n != 4 ||
                !(qq_things = q_quota->via.array->values) ||
                !(qq_props = q_quota->via.array->values + 1) ||
                !(qq_arrsz = q_quota->via.array->values + 2) ||
                !(qq_rawsz = q_quota->via.array->values + 3) ||
                (       qq_things->tp != QP_RES_INT64 &&
                        qq_things->tp != QP_RES_NULL) ||
                (       qq_props->tp != QP_RES_INT64 &&
                        qq_props->tp != QP_RES_NULL) ||
                (       qq_arrsz->tp != QP_RES_INT64 &&
                        qq_arrsz->tp != QP_RES_NULL) ||
                (       qq_rawsz->tp != QP_RES_INT64 &&
                        qq_rawsz->tp != QP_RES_NULL))
            goto stop;

        /* copy and check guid, must be null terminated */
        memcpy(guid.guid, qguid->via.raw->data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto stop;

        name = (char *) qname->via.raw->data;
        n = qname->via.raw->n;

        collection = ti_collection_create(&guid, name, n);
        if (!collection || vec_push(&ti()->collections->vec, collection))
            goto stop;

        collection->quota->max_things = qq_things->tp == QP_RES_NULL
                ? TI_QUOTA_NOT_SET
                : (size_t) qq_things->via.int64;
        collection->quota->max_props = qq_props->tp == QP_RES_NULL
                ? TI_QUOTA_NOT_SET
                : (size_t) qq_props->via.int64;
        collection->quota->max_array_size = qq_arrsz->tp == QP_RES_NULL
                ? TI_QUOTA_NOT_SET
                : (size_t) qq_arrsz->via.int64;
        collection->quota->max_raw_size = qq_rawsz->tp == QP_RES_NULL
                ? TI_QUOTA_NOT_SET
                : (size_t) qq_rawsz->via.int64;
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
