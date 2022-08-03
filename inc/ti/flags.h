/*
 * ti/flags.h
 */
#ifndef TI_FLAGS_H_
#define TI_FLAGS_H_

#include <inttypes.h>
#include <ex.h>
#include <ti/val.t.h>


#define TI_FLAGS_QUERY_MASK 0x1f

/* flags must fit with `TI_QUERY_FLAG` defined in query.t.h */
enum
{
    TI_FLAGS_NO_IDS=1<<5,   /* return no ID's */
};

int ti_flags_set_from_val(ti_val_t * val, uint8_t * flags, ex_t * e);

#endif  /* TI_FLAGS_H_ */
