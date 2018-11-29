/*
 * db.h
 */
#ifndef TI_DB_H_
#define TI_DB_H_

typedef struct ti_db_s  ti_db_t;

#include <uv.h>
#include <stdint.h>
#include <ti/thing.h>
#include <ti/raw.h>
#include <util/imap.h>
#include <util/guid.h>
#include <ti/ex.h>
#include <ti/quota.h>

ti_db_t * ti_db_create(guid_t * guid, const char * name, size_t n);
void ti_db_drop(ti_db_t * db);
_Bool ti_db_name_check(const char * name, size_t n, ex_t * e);
int ti_db_store(ti_db_t * db, const char * fn);
int ti_db_restore(ti_db_t * db, const char * fn);
static inline void * ti_db_thing_by_id(ti_db_t * db, uint64_t thing_id);

struct ti_db_s
{
    uint32_t ref;
    guid_t guid;            /* derived from db->root->id */
    ti_raw_t * name;
    imap_t * things;        /* weak map for ti_thing_t */
    vec_t * access;
    ti_thing_t * root;
    ti_quota_t * quota;
    uv_mutex_t * lock;      /* only for watch/unwatch/away-mode */
};

/* returns a borrowed reference */
static inline void * ti_db_thing_by_id(ti_db_t * db, uint64_t thing_id)
{
    return imap_get(db->things, thing_id);
}
#endif /* TI_DB_H_ */
