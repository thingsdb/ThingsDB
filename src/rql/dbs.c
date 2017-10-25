/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <assert.h>
#include <string.h>
#include <rql/dbs.h>
#include <rql/proto.h>
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
    qp_unpacker_t unpacker;
    qpx_packer_t * packer = qpx_packer_create(64);
    if (!packer)
    {
        ex_set_alloc(e);
        return;
    }

    qpx_unpacker_init(&unpacker, pkg->data, pkg->n);

    if (!qp_is_map(qp_next(&unpacker, NULL)) ||
        !qp_is_raw(qp_next(&unpacker, &target)))
    {
        ex_set(e, RQL_PROTO_TYPE_ERR, "invalid event");
        return;
    }
    assert (0 && sock); /* TODO: how should a get request look like? */
}
