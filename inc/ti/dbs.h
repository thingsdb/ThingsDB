/*
 * dbs.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_DBS_H_
#define TI_DBS_H_

#include <ti/db.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <ti/sock.h>
lude <util/ex.h>
#include <util/vec.h>

ti_db_t * ti_dbs_get_by_name(const vec_t * dbs, const ti_raw_t * name);
ti_db_t * ti_dbs_get_by_obj(const vec_t * dbs, const qp_obj_t * target);
void ti_dbs_get(ti_sock_t * sock, ti_pkg_t * pkg, ex_t * e);
int ti_dbs_store(const vec_t * dbs, const char * fn);
int ti_dbs_restore(vec_t ** dbs, const char * fn);

#endif /* TI_DBS_H_ */
