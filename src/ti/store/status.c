/*
 * ti/store/status.c
 */
#include <ti.h>
#include <util/qpx.h>
#include <util/fx.h>
#include <ti/store/status.h>

/* status of event id, thing id */
const int ti__stat_fn_schema = 0;


int ti_store_status_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(64);
    if (!packer) return -1;

    if (qp_add_map(&packer) ||
        qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, ti__stat_fn_schema) ||
        qp_add_raw_from_str(packer, "commit_event_id") ||
        qp_add_int64(packer, (int64_t) ti()->events->commit_event_id) ||
        qp_add_raw_from_str(packer, "next_thing_id") ||
        qp_add_int64(packer, (int64_t) ti()->next_thing_id) ||
        qp_close_map(packer)) goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc) log_error("failed to write file: `%s`", fn);
    qp_packer_destroy(packer);
    return rc;
}

int ti_store_status_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    uchar * data = fx_read(fn, &n);
    if (!data)
    {
        log_critical("failed to restore from file: `%s`", fn);
        return -1;
    }

    qp_unpacker_t unpacker;
    qpx_unpacker_init(&unpacker, data, (size_t) n);
    qp_res_t * res = qp_unpacker_res(&unpacker, &rcode);
    free(data);

    if (rcode)
    {
        log_critical(qp_strerror(rcode));
        return -1;
    }

    qp_res_t * schema, * qpcommit_event_id, * qpnext_thing_id;

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qpcommit_event_id = qpx_map_get(res->via.map, "commit_event_id")) ||
        !(qpnext_thing_id = qpx_map_get(res->via.map, "next_thing_id")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != ti__stat_fn_schema ||
        qpcommit_event_id->tp != QP_RES_INT64 ||
        qpnext_thing_id->tp != QP_RES_INT64)
        goto stop;

    ti()->events->commit_event_id = (uint64_t) qpcommit_event_id->via.int64;
    ti()->events->next_event_id = ti()->events->commit_event_id + 1;
    ti()->next_thing_id = (uint64_t) qpnext_thing_id->via.int64;

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
