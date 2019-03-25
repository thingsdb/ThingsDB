/*
 * ti/quorum.h
 */
#ifndef TI_QUORUM_H_
#define TI_QUORUM_H_

typedef struct ti_quorum_s ti_quorum_t;
typedef struct ti_quorum_res_s ti_quorum_res_t;

#include <uv.h>
#include <ti/req.h>
#include <util/vec.h>
#include <ti/ex.h>

typedef void (*ti_quorum_cb)(void * data, _Bool accepted);

ti_quorum_t * ti_quorum_new(ti_quorum_cb cb, void * data);
static inline void ti_quorum_destroy(ti_quorum_t * quorum);
static inline void ti_quorum_set_id(ti_quorum_t * quorum, uint64_t id);
void ti_quorum_go(ti_quorum_t * quorum);
void ti_quorum_req_cb(ti_req_t * req, ex_enum status);
static inline int ti_quorum_shrink_one(ti_quorum_t * quorum);

struct ti_quorum_s
{
    uint64_t id;
    int16_t rnode_id;
    uint8_t n;
    uint8_t accepted;
    uint8_t sz;
    uint8_t nn;
    uint8_t quorum;
    void * data;
    ti_quorum_cb cb_;
    ti_req_t * requests[];
};

/* only call this if something goes wrong before making the requests,
 * after `ti_quorum_go` the quorum will be destroyed when finished.
 */
static inline void ti_quorum_destroy(ti_quorum_t * quorum)
{
    free(quorum);
}

static inline void ti_quorum_set_id(ti_quorum_t * quorum, uint64_t id)
{
    quorum->id = id;
}

static inline int ti_quorum_shrink_one(ti_quorum_t * quorum)
{
    return quorum->sz ? --quorum->sz >= quorum->quorum ? 0 : -1 : -1;
}

#endif /* TI_QUORUM_H_ */
