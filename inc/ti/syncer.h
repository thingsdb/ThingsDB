/*
 * ti/syncer.h
 */
#ifndef TI_SYNCER_H_
#define TI_SYNCER_H_

typedef struct ti_syncer_s ti_syncer_t;

#include <ti/pkg.h>
#include <ti/ex.h>
#include <ti/user.h>
#include <ti/stream.h>
#include <util/logger.h>

ti_syncer_t * ti_syncer_create(void);
void ti_syncer_destroy(ti_syncer_t * syncer);


struct ti_syncer_s
{
    ti_stream_t * stream;       /* weak reference */

};


#endif  /* TI_SYNCER_H_ */
