/*
 * dbs.h
 */
#ifndef THINGSDB_DBS_H_
#define THINGSDB_DBS_H_

#include <ti/db.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <util/ex.h>

int thingsdb_dbs_create(void);
void thingsdb_dbs_destroy(void);
ti_db_t * thingsdb_dbs_get_by_name(const ti_raw_t * name);
ti_db_t * thingsdb_dbs_get_by_obj(const qp_obj_t * target);
void thingsdb_dbs_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e);
int thingsdb_dbs_store(const char * fn);
int thingsdb_dbs_restore(const char * fn);

#endif /* THINGSDB_DBS_H_ */
