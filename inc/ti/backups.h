/*
 * ti/backups.h
 */
#ifndef TI_BACKUPS_H_
#define TI_BACKUPS_H_


#include <uv.h>
#include <util/omap.h>

typedef struct ti_backups_s ti_backups_t;

int ti_backups_create(void);
void ti_backups_destroy(void);


struct ti_backups_s
{
    omap_t * omap;
    uv_mutex_t * lock;
};


#endif  /* TI_BACKUPS_H_ */
