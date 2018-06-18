/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <rql/db.h>
#include <rql/dbs.h>
#include <rql/proto.h>
#include <rql/inliners.h>
#include <util/qpx.h>
#include <util/fx.h>

const int rql_dbs_fn_schema = 0;

rql_db_t * rql_dbs_get_by_name(const vec_t * dbs, const rql_raw_t * name)
{
    for (vec_each(dbs, rql_db_t, db))
    {
        if (rql_raw_equal(db->name, name)) return db;
    }
    return NULL;
}

rql_db_t * rql_dbs_get_by_obj(const vec_t * dbs, const qp_obj_t * target)
{
    for (vec_each(dbs, rql_db_t, db))
    {
        if (qpx_obj_eq_raw(target, db->name) ||
            qpx_obj_eq_str(target, db->guid.guid))
        {
            return db;
        }
    }
    return NULL;
}

void rql_dbs_get(rql_sock_t * sock, rql_pkg_t * pkg, ex_t * e)
{
    qp_obj_t target;
    qp_obj_t qid;
    qp_unpacker_t unpacker;

    qpx_unpacker_init(&unpacker, pkg->data, pkg->n);

    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &target)) ||
        !qp_is_int(qp_next(&unpacker, &qid)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "invalid get request");
        return;
    }

    rql_db_t * db = rql_dbs_get_by_obj(sock->rql->dbs, &target);
    if (!db)
    {
        ex_set(e, RQL_PROTO_INDX_ERR, "cannot find database: '%.*s'",
                (int) target.len, target.via.raw);
        return;
    }

    uint64_t id = (uint64_t) qid.via.int64;

    rql_elem_t * elem = (qid.via.int64 < 0) ?
            db->root : (rql_elem_t *) imap_get(db->elems, id);

    if (!elem)
    {
        ex_set(e, RQL_PROTO_INDX_ERR, "cannot find element: %"PRIu64, id);
        return;
    }

    if (rql_has_id(sock->rql, elem->id))
    {
        qpx_packer_t * packer = qpx_packer_create(64);
        if (!packer || rql_elem_to_packer(elem, packer))
        {
            ex_set_alloc(e);
            return;
        }

        rql_pkg_t * resp = qpx_packer_pkg(packer, RQL_PROTO_ELEM);
        resp->id = pkg->id;

        if (!resp || rql_front_write(sock, resp))
        {
            free(resp);
            log_error(EX_ALLOC);
        }

        return;
    }

    assert (0 && sock); /* TODO: how should a get request look like? */
}

int rql_dbs_store(const vec_t * dbs, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return -1;

    if (qp_add_map(&packer)) goto stop;

    /* schema */
    if (qp_add_raw_from_str(packer, "schema") ||
        qp_add_int64(packer, rql_dbs_fn_schema)) goto stop;

    if (qp_add_raw_from_str(packer, "dbs") ||
        qp_add_array(&packer)) goto stop;

    for (vec_each(dbs, rql_db_t, db))
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

int rql_dbs_restore(vec_t ** dbs, rql_t * rql, const char * fn)
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
        schema->via.int64 != rql_dbs_fn_schema ||
        qdbs->tp != QP_RES_ARRAY) goto stop;

    for (uint32_t i = 0; i < qdbs->via.array->n; i++)
    {
        guid_t guid;
        rql_db_t * db;
        qp_res_t * qdb = qdbs->via.array->values + i;
        qp_res_t * qguid, * qname;
        if (qdb->tp != QP_RES_ARRAY ||
                qdb->via.array->n != 2 ||
            !(qguid = qdb->via.array->values) ||
            !(qname = qdb->via.array->values + 1) ||
            qguid->tp != QP_RES_RAW ||
            qguid->via.raw->n != sizeof(guid_t) ||
            qname->tp != QP_RES_RAW) goto stop;

        /* copy and check guid, must be null terminated */
        memcpy(guid.guid, qguid->via.raw->data, sizeof(guid_t));
        if (guid.guid[sizeof(guid_t) - 1]) goto stop;

        db = rql_db_create(rql, &guid, qname->via.raw);
        if (!db || vec_push(dbs, db)) goto stop;
    }

    rc = 0;

stop:
    if (rc) log_critical("failed to restore from file: `%s`", fn);
    qp_res_destroy(res);
    return rc;
}
