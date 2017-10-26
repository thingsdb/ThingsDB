/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <rql/dbs.h>
#include <rql/proto.h>
#include <rql/inliners.h>
#include <util/qpx.h>

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
    rql_elem_t * elem = (rql_elem_t *) imap_get(db->elems, id);
    if (!elem)
    {
        ex_set(e, RQL_PROTO_INDX_ERR, "cannot find element: %"PRIu64, id);
        return;
    }

    if (rql_has_id(sock->rql, id))
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
