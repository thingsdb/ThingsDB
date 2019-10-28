/*
 * ti/quorum.h
 */
#ifndef TI_QUORUM_H_
#define TI_QUORUM_H_

typedef struct ti_quorum_s ti_quorum_t;

#include <uv.h>
#include <ti/req.h>
#include <ex.h>

typedef void (*ti_quorum_cb)(void * data, _Bool accepted);

ti_quorum_t * ti_quorum_new(ti_quorum_cb cb, void * data);
static inline void ti_quorum_destroy(ti_quorum_t * quorum);
void ti_quorum_go(ti_quorum_t * quorum);
void ti_quorum_req_cb(ti_req_t * req, ex_enum status);
static inline int ti_quorum_shrink_one(ti_quorum_t * quorum);

struct ti_quorum_s
{
    int8_t diff_requests;       /* difference in requests between collisions */
    uint8_t accepted;           /* accepted answers */
    uint8_t rejected;           /* number of rejected answers */
    uint8_t collisions;         /* number of collisions */
    uint8_t requests;           /* number of requests (expected answers) */
    uint8_t quorum;             /* minimal required accessible nodes */
    uint8_t win_collision;      /* true when lowest collision id */
    uint8_t accept_threshold;   /* minimal required accepted */
    ti_quorum_cb cb_;           /* store the callback function */
    void * data;                /* public data binding */
};

/* only call this if something goes wrong before making the requests,
 * after `ti_quorum_go` the quorum will be destroyed when finished.
 */
static inline void ti_quorum_destroy(ti_quorum_t * quorum)
{
    free(quorum);
}

/*
 * Returns `0` if successful or `-1` if the quorum can no longer be reached
 */
static inline int ti_quorum_shrink_one(ti_quorum_t * quorum)
{
    quorum->accept_threshold = quorum->requests / 2;
    return (quorum->requests && --quorum->requests >= quorum->quorum) ? 0 : -1;
}

#endif /* TI_QUORUM_H_ */
