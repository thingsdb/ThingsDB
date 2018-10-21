/*
 * dbs.h
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ti/db.h>
#include <ti/dbs.h>
#include <ti/proto.h>
#include <ti.h>
#include <util/qpx.h>
#include <util/fx.h>
#include <util/vec.h>

static vec_t ** dbs = NULL;
static const int ti__dbs_fn_schema = 0;

int ti_dbs_create(void)
{
    ti_get()->dbs = vec_new(0);
    dbs = &ti_get()->dbs;
    return -(*dbs == NULL);
}

void ti_dbs_destroy(void)
{
    if (!dbs)
        return;
    vec_destroy(*dbs, (vec_destroy_cb) ti_db_drop);
    *dbs = NULL;
}

ti_db_t * ti_dbs_get_by_name(const ti_raw_t * name)
{
    for (vec_each(*dbs, ti_db_t, db))
    {
        if (ti_raw_equal(db->name, name)) return db;
    }
    return NULL;
}

ti_db_t * ti_dbs_get_by_obj(const qp_obj_t * target)
{
    for (vec_each(*dbs, ti_db_t, db))
    {
        if (qpx_obj_eq_raw(target, db->name) ||
            qpx_obj_eq_str(target, db->guid.guid))
        {
            return db;
        }
    }
    return NULL;
}

//void ti_dbs_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e)
//{
//    qp_obj_t target;
//    qp_obj_t qid;
//    qp_unpacker_t unpacker;
//
//    qpx_unpacker_init(&unpacker, pkg->data, pkg->n);
//
//    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
//        !qp_is_raw(qp_next(&unpacker, &target)) ||
//        !qp_is_int(qp_next(&unpacker, &qid)))
//    {
//        ex_set(e, TI_PROTO_TYPE_ERR, "invalid get request");
//        return;
//    }
//
//    ti_db_t * db = ti_dbs_get_by_obj(&target);
//    if (!db)
//    {
//        ex_set(e, TI_PROTO_INDX_ERR, "cannot find database: `%.*s`",
//                (int) target.len, target.via.raw);
//        return;
//    }
//
//    uint64_t id = (uint64_t) qid.via.int64;
//
//    ti_thing_t * thing = (qid.via.int64 < 0) ?
//            db->root : (ti_thing_t *) imap_get(db->things, id);
//
//    if (!thing)
//    {
//        ex_set(e, TI_PROTO_INDX_ERR, "cannot find thing: `%"PRIu64"`", id);
//        return;
//    }
//
//    if (ti_manages_id(thing->id))
//    {
//        qpx_packer_t * packer = qpx_packer_create(64);
//        if (!packer || ti_thing_to_packer(thing, packer))
//        {
//            ex_set_alloc(e);
//            return;
//        }
//
//        ti_pkg_t * resp = qpx_packer_pkg(packer, TI_PROTO_ELEM);
//        resp->id = pkg->id;
//
//        if (!resp || ti_front_write(sock, resp))
//        {
//            free(resp);
//            log_error(EX_ALLOC);
//        }
//
//        return;
//    }
//
//    assert (0 && sock); /* TODO: how should a get request look like? */
//}

int ti_dbs_store(const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return -1;

    if (qp_add_map(&packer)) goto stop;

    /* schema */
    if (qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, ti__dbs_fn_schema)) goto stop;

    if (qp_add_raw_from_str(packer, "dbs") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(*dbs, ti_db_t, db))
    {
        if (qp_add_array(&packer) ||
            qp_add_raw(packer,
                    (const unsigned char *) db->guid.guid, sizeof(guid_t)) ||
            qp_add_raw(packer, db->name->data, db->name->n) ||
            qp_close_array(packer)) goto stop;
    }

    if (qp_close_array(packer) || qp_close_map(packer)) goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc) log_error("failed to write file: `%s`", fn);
    qp_packer_destroy(packer);
    return rc;
}

int ti_dbs_restore(const char * fn)
{
    int rcode, rc = -1;
    ssize_t n;
    unsigned char * data = fx_read(fn, &n);
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

    if (res->tp != QP_RES_MAP ||
        !(schema = qpx_map_get(res->via.map, "schema")) ||
        !(qdbs = qpx_map_get(res->via.map, "dbs")) ||
        schema->tp != QP_RES_INT64 ||
        schema->via.int64 != ti__dbs_fn_schema ||
        qdbs->tp != QP_RES_ARRAY) goto stop;

    for (uint32_t i = 0; i < qdbs->via.array->n; i++)
    {
        guid_t guid;
        ti_db_t * db;
        qp_res_t * qdb = qdbs->via.array->values + i;
        qp_res_t * qguid, * qname;
        if (    qdb->tp != QP_RES_ARRAY ||
                qdb->via.array->n != 2 ||
                !(qguid = qdb->via.array->values) ||
                !(qname = qdb->via.array->values + 1) ||
                qguid->tp != QP_RES_RAW ||
                qguid->via.raw->n != sizeof(guid_t) ||
                qname->tp != QP_RES_RAW)
            goto stop;

        /* copy and check guid, must be null terminated */
        memcpy(guid.guid, qguid->via.raw->data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1])
            goto stop;

        db = ti_db_create(&guid, qname->via.raw);
        if (!db || vec_push(dbs, db))
            goto stop;
    }

    rc = 0;

stop:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
