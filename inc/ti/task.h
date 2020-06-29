/*
 * ti/task.h
 */
#ifndef TI_TASK_H_
#define TI_TASK_H_

#include <inttypes.h>
#include <ti/collection.t.h>
#include <ti/enum.t.h>
#include <ti/field.t.h>
#include <ti/member.t.h>
#include <ti/method.t.h>
#include <ti/name.t.h>
#include <ti/pkg.t.h>
#include <ti/procedure.h>
#include <ti/task.t.h>
#include <ti/thing.t.h>
#include <ti/token.t.h>
#include <ti/type.t.h>
#include <ti/user.t.h>
#include <ti/val.t.h>
#include <ti/varr.t.h>
#include <util/vec.h>

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing);
ti_task_t * ti_task_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e);
void ti_task_destroy(ti_task_t * task);
ti_pkg_t * ti_task_pkg_watch(ti_task_t * task);
int ti_task_add_add(ti_task_t * task, ti_name_t * name, vec_t * added);
int ti_task_add_set(ti_task_t * task, ti_name_t * name, ti_val_t * val);
int ti_task_add_new_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_set_type(ti_task_t * task, ti_type_t * type);
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
int ti_task_add_mod_type_add_field(
        ti_task_t * task,
        ti_type_t * type,
        ti_val_t * dval);
int ti_task_add_mod_type_add_method(
        ti_task_t * task,
        ti_type_t * type);
int ti_task_add_mod_type_del(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * name);
int ti_task_add_mod_type_mod_field(ti_task_t * task, ti_field_t * field);
int ti_task_add_mod_type_mod_method(
        ti_task_t * task,
        ti_type_t * type,
        ti_method_t * method);
int ti_task_add_mod_type_ren(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * oldname,
        ti_name_t * newname);
int ti_task_add_del_node(ti_task_t * task, uint32_t node_id);
int ti_task_add_remove(ti_task_t * task, ti_name_t * name, vec_t * removed);
int ti_task_add_rename_collection(
        ti_task_t * task,
        ti_collection_t * collection);
int ti_task_add_rename_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_revoke(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask);
int ti_task_add_set_password(ti_task_t * task, ti_user_t * user);
int ti_task_add_splice(
        ti_task_t * task,
        ti_name_t * name,
        ti_varr_t * varr,       /* array or array-of-things */
        uint32_t i,              /* start at index */
        uint32_t c,              /* number of items to remove */
        uint32_t n);             /* number of items to add */
int ti_task_add_restore(ti_task_t * task);
int ti_task_add_set_enum(ti_task_t * task, ti_enum_t * enum_);
int ti_task_add_mod_enum_add(ti_task_t * task, ti_member_t * member);
int ti_task_add_mod_enum_def(ti_task_t * task, ti_member_t * member);
int ti_task_add_mod_enum_del(ti_task_t * task, ti_member_t * member);
int ti_task_add_mod_enum_mod(ti_task_t * task, ti_member_t * member);
int ti_task_add_mod_enum_ren(ti_task_t * task, ti_member_t * member);
int ti_task_add_del_enum(ti_task_t * task, ti_enum_t * enum_);
int ti_task_add_event(
        ti_task_t * task,
        ti_raw_t * revent,
        vec_t * vec,
        int deep);

#endif /* TI_TASK_H_ */
