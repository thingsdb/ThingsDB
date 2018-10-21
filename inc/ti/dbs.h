/*
 * dbs.h
 */
#ifndef TI_DBS_H_
#define TI_DBS_H_

#include <ti/db.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <util/ex.h>

int ti_dbs_create(void);
void ti_dbs_destroy(void);
ti_db_t * ti_dbs_get_by_name(const ti_raw_t * name);
ti_db_t * ti_dbs_get_by_obj(const qp_obj_t * target);
void ti_dbs_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e);
int ti_dbs_store(const char * fn);
int ti_dbs_restore(const char * fn);

#endif /* TI_DBS_H_ */
