/*
 * ti/commit.h
 */
#ifndef TI_COMMIT_H_
#define TI_COMMIT_H_

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <util/vec.h>

typedef struct ti_commit_s ti_commit_t;

ti_commit_t * ti_commit_create(
        uint64_t id,
        time_t ts,
        const char * code,
        size_t code_n,
        const char * message,
        size_t message_n,
        const char * by,
        size_t by_n,
        const char * err_msg,  /* may be NULL*/
        size_t err_msg_n);
ti_commit_t * ti_commit_make(
        uint64_t id,
        const char * code,
        ti_raw_t * by,
        ti_raw_t * message);
void ti_commit_destroy(ti_commit_t * commit);

struct ti_commit_s
{
    uint64_t id;        /* equal to the change id */
    time_t ts;          /* timestamp of the change */
    ti_raw_t * code;        /* original query string */
    ti_raw_t * message;     /* never NULL */
    ti_raw_t * by;          /* never NULL */
    ti_raw_t * err_msg;     /* may be NULL */
}

#endif /* TI_COMMIT_H_ */
