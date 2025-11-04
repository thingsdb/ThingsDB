/*
 * ti/commit.h
 */
#ifndef TI_COMMIT_H_
#define TI_COMMIT_H_

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ti/raw.t.h>
#include <ti/val.t.h>
#include <util/mpack.h>
#include <util/vec.h>

typedef struct ti_commit_s ti_commit_t;

ti_commit_t * ti_commit_from_up(mp_unp_t * up);
ti_commit_t * ti_commit_make(
        uint64_t id,
        const char * code,
        ti_raw_t * by,
        ti_raw_t * message);
void ti_commit_destroy(ti_commit_t * commit);
int ti_commit_to_pk(ti_commit_t * commit, msgpack_packer * pk);
int ti_commit_to_client_pk(
        ti_commit_t * commit,
        _Bool detail,
        msgpack_packer * pk);
ti_val_t * ti_commit_as_mpval(ti_commit_t * commit, _Bool detail);

struct ti_commit_s
{
    uint64_t id;        /* equal to the change id */
    time_t ts;          /* timestamp of the change */
    ti_raw_t * code;        /* original query string */
    ti_raw_t * message;     /* never NULL */
    ti_raw_t * by;          /* never NULL */
    ti_raw_t * err_msg;     /* may be NULL */
};

#endif /* TI_COMMIT_H_ */
