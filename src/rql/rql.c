/*
 * rql.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

#include <rql/rql.h>
#include <rql/db.h>
#include <util/logger.h>
#include <util/strx.h>
#include <util/filex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

rql_t * rql_create(void)
{
    rql_t * rql = (rql_t *) malloc(sizeof(rql_t));
    if (!rql) return NULL;

    rql->redundancy = rql_def_redundancy;
    rql->node = NULL;

    rql->fn = NULL;
    rql->args = rql_args_create();
    rql->cfg = rql_cfg_create();
    rql->dbs = vec_create(0);
    rql->nodes = vec_create(0);
    rql->users = vec_create(0);
    rql->loop = (uv_loop_t *) malloc(sizeof(uv_loop_t));

    if (!rql->args ||
        !rql->cfg ||
        !rql->dbs ||
        !rql->nodes ||
        !rql->users ||
        !rql->loop)
    {
        rql_destroy(rql);
        return NULL;
    }

    return rql;
}

void rql_destroy(rql_t * rql)
{
    if (!rql) return;

    free(rql->fn);
    rql_args_destroy(rql->args);
    rql_cfg_destroy(rql->cfg);
    vec_destroy(rql->dbs, (vec_destroy_cb) rql_db_drop);
    vec_destroy(rql->nodes, NULL);
    vec_destroy(rql->users, NULL);
    free(rql->loop);

    free(rql);
}

void rql_init_logger(rql_t * rql)
{
    int n;
    char lname[255];
    size_t len = strlen(rql->args->log_level);

#ifndef DEBUG
    /* force colors while debugging... */
    if (rql->args->log_colorized)
#endif
    {
        Logger.flags |= LOGGER_FLAG_COLORED;
    }

    for (n = 0; n < LOGGER_NUM_LEVELS; n++)
    {
        strcpy(lname, LOGGER_LEVEL_NAMES[n]);
        strx_lower_case(lname);
        if (strlen(lname) == len && strcmp(rql->args->log_level, lname) == 0)
        {
            logger_init(stdout, n);
            return;
        }
    }
    /* We should not get here since args should always
     * contain a valid log level
     */
    logger_init(stdout, 0);
}

int rql_init_fn(rql_t * rql)
{
    rql->fn = strx_cat(rql->cfg->rql_path, rql_fn);
    return (rql->fn) ? 0 : -1;
}

int rql_build(rql_t * rql)
{
    rql_user_t * user = rql_user_create(rql_user_def_name);
    if (!user || vec_append(rql->users, user)) goto failed;

    rql->node = rql_node_create(0, rql->cfg->node_address);
    if (!rql->node || vec_append(rql->nodes, rql->node)) goto failed;

    if (rql_save(rql)) goto failed;

    return 0;

failed:
    vec_pop(rql->users);
    rql_user_drop(user);
    vec_pop(rql->nodes);
    rql_node_drop(rql->node);
    rql->node = NULL;
    return -1;
}

int rql_read(rql_t * rql)
{
    int rc;
    ssize_t n;
    unsigned char * data = filex_read(rql->fn, &n);
    if (!data) return -1;
    qp_unpacker_t unpacker;
    qp_unpacker_init(&unpacker, data, (size_t) n);
    qp_res_t * res = qp_unpacker_res(&unpacker, &rc);
    free(data);
    if (rc)
    {
        log_critical(qp_strerror(rc));
        return -1;
    }
    rc = rql_unpack(rql, res);
    qp_res_destroy(res);
    return rc;
}

int rql_save(rql_t * rql)
{
    qp_packer_t * packer = rql_pack(rql);
    if (!packer) return -1;

    int rc = filex_write(rql->fn, packer->buffer, packer->len);
    qp_packer_destroy(packer);
    return rc;
}

int rql_unpack(rql_t * rql, qp_res_t * res)
{
    qp_res_t * redundancy, * node, * nodes;

    if (res->tp != QP_RES_MAP ||
        !(redundancy = qpx_map_get(res->via.map, "redundancy")) ||
        !(node = qpx_map_get(res->via.map, "node")) ||
        !(nodes = qpx_map_get(res->via.map, "nodes")) ||
        redundancy->tp != QP_RES_INT64 ||
        node->tp != QP_RES_INT64 ||
        nodes->tp != QP_RES_ARRAY ||
        redundancy->via.int64 < 1 ||
        redundancy->via.int64 > 64 ||
        node->via.int64 < 0) goto failed;

    rql->redundancy = (uint8_t) redundancy->via.int64;

    for (uint32_t i = 0; i < nodes->via.array->n; i++)
    {
        qp_res_t * itm = nodes->via.array->values[i];
        qp_res_t * addr, * port;
        if (itm->tp != QP_RES_ARRAY ||
            itm->via.array->n != 2 ||
            !(addr = itm->via.array->values[0]) ||
            !(port = itm->via.array->values[1]) ||
            addr->tp != QP_RES_STR ||
            port->tp != QP_RES_INT64 ||
            port->via.int64 < 1 ||
            port->via.int64 > 65535) goto failed;

        rql_node_t * node = rql_node_create(
                (uint8_t) i,
                addr->via.str,
                (uint16_t) port->via.int64);

        if (!node || vec_append(rql->nodes, node)) goto failed;
    }

    if (node->via.int64 >= rql->nodes->n) goto failed;

    rql->node = (rql_node_t *) vec_get(rql->nodes, (uint32_t) node->via.int64);

    return 0;

failed:
    for (uint32_t i = 0; i < rql->nodes->n; i++)
    {
        rql_node_t * node = (rql_node_t *) vec_get(rql->nodes, i);
        rql_node_drop(node);
    }
    rql->node = NULL;
    return -1;
}

qp_packer_t * rql_pack(rql_t * rql)
{
    qp_packer_t * packer = qp_packer_create(1024);
    if (!packer) return NULL;

    if (qp_add_map(&packer)) goto failed;

    /* redundancy */
    if (qp_add_raw(packer, "redundancy", 10) ||
        qp_add_int64(packer, (int64_t) rql->redundancy)) goto failed;

    /* node */
    if (qp_add_raw(packer, "node", 4) ||
        qp_add_int64(packer, (int64_t) rql->node->id)) goto failed;

    /* nodes */
    if (qp_add_raw(packer, "nodes", 5) || qp_add_array(&packer)) goto failed;
    for (uint32_t i = 0; i < rql->nodes->n; i++)
    {
        rql_node_t * node = (rql_node_t *) vec_get(rql->nodes, i);
        if (qp_add_array(&packer) ||
            qp_add_raw(packer, node->addr, strlen(node->addr)) ||
            qp_add_int64(packer, (int64_t) node->port) ||
            qp_close_array(packer)) goto failed;

    }
    if (qp_close_array(packer)) goto failed;

    if (qp_close_map(packer)) goto failed;

    return packer;

failed:
    qp_packer_destroy(packer);
    return NULL;
}
