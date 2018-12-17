/*
 * ti/sync.h
 */
#ifndef TI_SYNC_H_
#define TI_SYNC_H_

typedef struct ti_sync_s ti_sync_t;

#include <ti/pkg.h>
#include <ti/ex.h>
#include <ti/user.h>
#include <ti/stream.h>
#include <util/logger.h>

void ti_sync_create(void);
void ti_sync_destroy(void);


struct ti_sync_s
{
    uint8_t status;
};


#endif  /* TI_SYNC_H_ */
