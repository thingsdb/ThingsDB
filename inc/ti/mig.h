/*
 * ti/mig.h
 */
#ifndef TI_MIG_H_
#define TI_MIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ti/raw.t.h>
#include <util/mpack.h>
#include <util/vec.h>

typedef struct ti_mig_s ti_mig_t;

ti_mig_t * ti_mig_create(
        uint64_t id,
        time_t ts,
        const char * query,
        size_t query_n,
        const char * info,
        size_t info_n,
        const char * by,
        size_t by_n,
        const char * err_msg,  /* may be NULL*/
        size_t err_msg_n);
ti_mig_t * ti_mig_create_q(
        uint64_t id,
        const char * query,
        ti_raw_t * by);
void ti_mig_destroy(ti_mig_t * mig);
int ti_mig_set_history(vec_t ** migs, _Bool state);

struct ti_mig_s
{
    uint64_t id;        /* equal to the change id */
    time_t ts;          /* timestamp of the change */
    ti_raw_t * query;       /* original query string */
    ti_raw_t * info;        /* never NULL */
    ti_raw_t * by;          /* never NULL */
    ti_raw_t * err_msg;     /* may be NULL */
}

#endif /* TI_MIG_H_ */
