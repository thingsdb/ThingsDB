/*
 * ti/task.h
 */
#ifndef TI_TASK_H_
#define TI_TASK_H_

typedef enum
{
    TI_TASK_ASSIGN,
    TI_TASK_DEL,
    TI_TASK_PUSH,
    TI_TASK_SET,
    TI_TASK_UNSET,
} ti_task_enum;

typedef struct ti_task_s ti_task_t;

#include <inttypes.h>
#include <ti/thing.h>
#include <ti/name.h>
#include <ti/val.h>
#include <ti/type.h>
#include <ti/pkg.h>
#include <ti/collection.h>
#include <ti/user.h>
#include <ti/field.h>
#include <ti/token.h>
#include <ti/quota.h>
#include <ti/procedure.h>
#include <util/vec.h>

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing);
ti_task_t * ti_task_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e);
void ti_task_destroy(ti_task_t * task);
ti_pkg_t * ti_task_pkg_watch(ti_task_t * task);
int ti_task_add_add(ti_task_t * task, ti_name_t * name, vec_t * added);
int ti_task_add_set(ti_task_t * task, ti_name_t * name, ti_val_t * val);
int ti_task_add_new_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_del(ti_task_t * task, ti_raw_t * name);
int ti_task_add_del_collection(ti_task_t * task, uint64_t collection_id);
int ti_task_add_del_expired(ti_task_t * task, uint64_t after_ts);
int ti_task_add_del_procedure(ti_task_t * task, ti_raw_t * name);
int ti_task_add_del_token(ti_task_t * task, ti_token_key_t * key);
int ti_task_add_del_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_del_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_grant(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask);
int ti_task_add_new_collection(
        ti_task_t * task,
        ti_collection_t * collection,
        ti_user_t * user);
int ti_task_add_new_node(ti_task_t * task, ti_node_t * node);
int ti_task_add_new_procedure(ti_task_t * task, ti_procedure_t * procedure);
int ti_task_add_new_token(
        ti_task_t * task,
        ti_user_t * user,
        ti_token_t * token);
int ti_task_add_new_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_mod_type_add(
        ti_task_t * task,
        ti_type_t * type,
        ti_val_t * val);
int ti_task_add_mod_type_del(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * name);
int ti_task_add_mod_type_mod(
        ti_task_t * task,
        ti_field_t * field);
int ti_task_add_del_node(ti_task_t * task, uint32_t node_id);
int ti_task_add_remove(ti_task_t * task, ti_name_t * name, vec_t * removed);
int ti_task_add_rename_collection(
        ti_task_t * task,
        ti_collection_t * collection);
int ti_task_add_rename_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_replace_node(ti_task_t * task, ti_node_t * node);
int ti_task_add_revoke(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask);
int ti_task_add_set_password(ti_task_t * task, ti_user_t * user);
int ti_task_add_set_quota(
        ti_task_t * task,
        uint64_t collection_id,
        ti_quota_enum_t quota_tp,
        size_t quota);
int ti_task_add_splice(
        ti_task_t * task,
        ti_name_t * name,
        ti_varr_t * varr,       /* array or array-of-things */
        int64_t i,              /* start at index */
        int64_t c,              /* number of items to remove */
        int32_t n);             /* number of items to add */


struct ti_task_s
{
    uint64_t event_id;
    size_t approx_sz;
    ti_thing_t * thing;     /* with reference */
    vec_t * jobs;           /* q-pack (unsigned char *) */
};

#endif /* TI_TASK_H_ */
