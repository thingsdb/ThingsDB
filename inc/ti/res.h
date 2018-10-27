/*
 * ti/res.h - query response
 */
#ifndef TI_RES_H_
#define TI_RES_H_

typedef struct ti_res_s ti_res_t;

#include <ti/db.h>
#include <ti/val.h>
#include <util/imap.h>

struct ti_res_s
{
    ti_db_t * db;
    ti_val_t * val;
    imap_t * collect;
};

#endif /* TI_RES_H_ */
