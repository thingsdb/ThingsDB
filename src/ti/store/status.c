/*
 * ti/store/status.c
 */
#include <ti.h>
#include <util/qpx.h>
#include <util/fx.h>
#include <ti/store/status.h>

int ti_store_status_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(64);
    if (!packer)
        return -1;

    if (qp_add_map(&packer) ||
        qp_add_raw_from_str(packer, "cevid") ||
        qp_add_int64(packer, (int64_t) *ti()->events->cevid) ||
        qp_add_raw_from_str(packer, "next_thing_id") ||
        qp_add_int64(packer, (int64_t) *ti()->next_thing_id) ||
        qp_close_map(packer)
    )
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    else
        ti()->stored_event_id = *ti()->events->cevid;
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

    qp_res_t * qpcevid, * qpnext_thing_id;

    if (res->tp != QP_RES_MAP ||
        !(qpcevid = qpx_map_get(res->via.map, "cevid")) ||
        !(qpnext_thing_id = qpx_map_get(res->via.map, "next_thing_id")) ||
        qpcevid->tp != QP_RES_INT64 ||
        qpnext_thing_id->tp != QP_RES_INT64)
        goto stop;

    ti()->node->cevid = (uint64_t) qpcevid->via.int64;
    ti()->events->cevid = &ti()->node->cevid;
    ti()->stored_event_id = ti()->node->cevid;
    ti()->events->next_event_id = (*ti()->events->cevid) + 1;
    ti()->node->next_thing_id = (uint64_t) qpnext_thing_id->via.int64;
    ti()->next_thing_id = &ti()->node->next_thing_id;
    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
