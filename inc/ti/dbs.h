/*
 * dbs.h
 */
#ifndef TI_DBS_H_
#define TI_DBS_H_

typedef struct ti_dbs_s ti_dbs_t;

#include <ti/db.h>
#include <ti/pkg.h>
#include <ti/raw.h>
#include <ti/stream.h>
#include <qpack.h>
#include <ti/ex.h>
#include <ti/val.h>

int ti_dbs_create(void);
void ti_dbs_destroy(void);
ti_db_t * ti_dbs_create_db(
        const char * name,
        size_t n,
        ti_user_t * user,
        ex_t * e);
ti_db_t * ti_dbs_get_by_raw(const ti_raw_t * raw);
ti_db_t * ti_dbs_get_by_strn(const char * str, size_t n);
ti_db_t * ti_dbs_get_by_id(const uint64_t id);
ti_db_t * ti_dbs_get_by_qp_obj(qp_obj_t * obj, ex_t * e);
ti_db_t * ti_dbs_get_by_val(ti_val_t * val, ex_t * e);
void ti_dbs_get(ti_stream_t * sock, ti_pkg_t * pkg, ex_t * e);

struct ti_dbs_s
{
    vec_t * vec;        /* ti_db_t with reference */
};

#endif /* TI_DBS_H_ */
