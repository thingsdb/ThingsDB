/*
 * ti/nodes.h
 */
#ifndef TI_NODES_H_
#define TI_NODES_H_

#include <ex.h>
#include <uv.h>
#include <ti/node.h>
#include <ti/rpkg.h>
#include <util/vec.h>
#include <util/mpack.h>

typedef struct ti_nodes_s ti_nodes_t;

typedef enum
{
    TI_NODES_IGNORE_SYNC,
    TI_NODES_RETRY_OFFLINE,
    TI_NODES_WAIT_AWAY,
} ti_nodes_ignore_t;

int ti_nodes_create(void);
void ti_nodes_destroy(void);
int ti_nodes_read_sccid(void);
int ti_nodes_write_global_status(void);
int ti_nodes_listen(void);
uint8_t ti_nodes_quorum(void);
_Bool ti_nodes_has_quorum(void);
ti_node_t * ti_nodes_not_ready(void);
_Bool ti_nodes_offline_found(void);
void ti_nodes_write_rpkg(ti_rpkg_t * rpkg);
int ti_nodes_to_pk(msgpack_packer * pk);
int ti_nodes_from_up(mp_unp_t * up);
ti_nodes_ignore_t ti_nodes_ignore_sync(uint8_t retry_offline);
_Bool ti_nodes_require_sync(void);
int ti_nodes_check_add(ex_t * e);
uint64_t ti_nodes_ccid(void);
uint64_t ti_nodes_scid(void);
uint32_t ti_nodes_next_id(void);
void ti_nodes_update_syntax_ver(uint16_t syntax_ver);
ti_node_t * ti_nodes_new_node(
        uint32_t id,
        uint8_t zone,
        uint16_t port,
        const char * addr,
        const char * secret);
void ti_nodes_del_node(uint32_t node_id);
ti_node_t * ti_nodes_node_by_id(uint32_t node_id);
ti_node_t * ti_nodes_get_away(void);
ti_node_t * ti_nodes_get_away_or_soon(void);
ti_node_t * ti_nodes_random_ready_node(void);
void ti_nodes_set_not_ready_err(ex_t * e);
void ti_nodes_pkg_cb(ti_stream_t * stream, ti_pkg_t * pkg);
ti_varr_t * ti_nodes_info(void);
int ti_nodes_check_syntax(uint16_t syntax_ver, ex_t * e);

struct ti_nodes_s
{
    vec_t * vec;            /* store the nodes */
    uv_tcp_t tcp;
    uint64_t ccid;         /* last committed change id by ALL nodes,
                               ti_archive_t saves this value to disk at
                               cleanup and is therefore responsible to set
                               the initial value at startup */
    uint64_t scid;         /* last stored change id by ALL nodes,
                               ti_archive_t saves this value to disk at
                               cleanup and is therefore responsible to set
                               the initial value at startup */
    uint32_t next_id;       /* next node id */
    uint16_t syntax_ver;    /* lowest syntax version by ALL nodes */
    uint16_t pad0_;
    char * status_fn;       /* this file contains the last known committed
                               and stored change id's by ALL nodes, and the
                               lowest known syntax version */
};

#endif /* TI_NODES_H_ */
