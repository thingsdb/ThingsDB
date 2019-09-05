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
    uint8_t n;                  /* number of received answers */
    uint8_t accepted;           /* accepted answers */
    uint8_t sz;                 /* expected answers */
    uint8_t quorum;             /* minimal required accepted answers */
    uint8_t reject_threshold;   /* maximum rejected answers */
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
    return (quorum->sz && --quorum->sz >= quorum->quorum) ? 0 : -1;
}

#endif /* TI_QUORUM_H_ */
