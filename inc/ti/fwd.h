/*
 * ti/fwd.h
 */
#ifndef TI_FWD_H_
#define TI_FWD_H_


typedef struct ti_fwd_s ti_fwd_t;

#include <inttypes.h>
#include <ti/stream.h>


ti_fwd_t * ti_fwd_create(uint16_t orig_pkg_id, ti_stream_t * src_stream);
void ti_fwd_destroy(ti_fwd_t * fwd);

struct ti_fwd_s
{
    uint16_t orig_pkg_id;
    ti_stream_t * stream;
};

#endif  /* TI_FWD_H_ */
