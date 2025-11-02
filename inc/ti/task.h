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

ti_task_t * ti_task_create(uint64_t change_id, ti_thing_t * thing);
ti_task_t * ti_task_new_task(ti_change_t * change, ti_thing_t * thing);
ti_task_t * ti_task_get_task(ti_change_t * change, ti_thing_t * thing);
void ti_task_destroy(ti_task_t * task);
int ti_task_add_set_add(ti_task_t * task, ti_raw_t * key, vec_t * added);
int ti_task_add_thing_clear(ti_task_t * task);
int ti_task_add_arr_clear(ti_task_t * task, ti_raw_t * key);
int ti_task_add_set_clear(ti_task_t * task, ti_raw_t * key);
int ti_task_add_set(ti_task_t * task, ti_raw_t * key, ti_val_t * val);
int ti_task_add_new_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_to_thing(ti_task_t * task);
int ti_task_add_to_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_restrict(ti_task_t * task, uint16_t spec);
int ti_task_add_set_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_del(ti_task_t * task, ti_raw_t * name);
int ti_task_add_ren(ti_task_t * task, ti_raw_t * oname, ti_raw_t * nname);
int ti_task_add_del_collection(ti_task_t * task, uint64_t collection_id);
int ti_task_add_del_expired(ti_task_t * task, uint64_t after_ts);
int ti_task_add_del_procedure(ti_task_t * task, ti_raw_t * name);
int ti_task_add_del_token(ti_task_t * task, ti_token_key_t * key);
int ti_task_add_del_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_del_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_del_module(ti_task_t * task, ti_module_t * module);
int ti_task_add_deploy_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * mdata);
int ti_task_add_set_module_scope(ti_task_t * task, ti_module_t * module);
int ti_task_add_set_module_conf(ti_task_t * task, ti_module_t * module);
int ti_task_add_grant(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask);
int ti_task_add_new_collection(
        ti_task_t * task,
        ti_collection_t * collection,
        ti_user_t * user);
int ti_task_add_new_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * source);
int ti_task_set_name(ti_task_t * task, ti_room_t * room);
int ti_task_add_new_node(ti_task_t * task, ti_node_t * node);
int ti_task_add_new_procedure(ti_task_t * task, ti_procedure_t * procedure);
int ti_task_add_mod_procedure(ti_task_t * task, ti_procedure_t * procedure);
int ti_task_add_vtask_new(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_del(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_cancel(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_finish(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_set_args(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_set_owner(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_vtask_set_closure(ti_task_t * task, ti_vtask_t * vtask);
int ti_task_add_new_token(
        ti_task_t * task,
        ti_user_t * user,
        ti_token_t * token);
int ti_task_add_new_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_mod_type_add_field(
        ti_task_t * task,
        ti_type_t * type,
        ti_val_t * dval);
int ti_task_add_mod_type_add_idname(ti_task_t * task, ti_type_t * type);
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
int ti_task_add_mod_type_rel_add(
        ti_task_t * task,
        ti_type_t * type1,
        ti_name_t * name1,
        ti_type_t * type2,
        ti_name_t * name2);
int ti_task_add_mod_type_rel_del(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * name);
int ti_task_add_mod_type_ren(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * oldname,
        ti_name_t * newname);
int ti_task_add_mod_type_wpo(ti_task_t * task, ti_type_t * type);
int ti_task_add_mod_type_hid(ti_task_t * task, ti_type_t * type);
int ti_task_add_del_node(ti_task_t * task, uint32_t node_id);
int ti_task_add_set_remove(ti_task_t * task, ti_raw_t * key, vec_t * removed);
int ti_task_add_rename_collection(
        ti_task_t * task,
        ti_collection_t * collection);
int ti_task_add_set_time_zone(
        ti_task_t * task,
        uint64_t scope_id,
        uint64_t tz_index);
int ti_task_add_set_default_deep(
        ti_task_t * task,
        uint64_t scope_id,
        uint8_t deep);
int ti_task_add_rename_procedure(
        ti_task_t * task,
        ti_procedure_t * procedure,
        ti_raw_t * nname);
int ti_task_add_rename_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * nname);
int ti_task_add_rename_type(ti_task_t * task, ti_type_t * type);
int ti_task_add_rename_enum(ti_task_t * task, ti_enum_t * enum_);
int ti_task_add_rename_user(ti_task_t * task, ti_user_t * user);
int ti_task_add_revoke(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask);
int ti_task_add_set_password(ti_task_t * task, ti_user_t * user);
int ti_task_add_fill(ti_task_t * task, ti_raw_t * key, ti_val_t * val);
int ti_task_add_splice(
        ti_task_t * task,
        ti_raw_t * key,
        ti_varr_t * varr,       /* array or array-of-things */
        uint32_t i,              /* start at index */
        uint32_t c,              /* number of items to remove */
        uint32_t n);             /* number of items to add */
int ti_task_add_restore(ti_task_t * task);
int ti_task_add_import(ti_task_t * task, ti_raw_t * bytes, _Bool import_tasks);
int ti_task_add_arr_remove(ti_task_t * task, ti_raw_t * key, vec_t * vec);
int ti_task_add_thing_remove(ti_task_t * task, vec_t * vec, size_t alloc_sz);
int ti_task_add_set_enum(ti_task_t * task, ti_enum_t * enum_);
int ti_task_add_mod_enum_add(
        ti_task_t * task,
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val);
int ti_task_add_mod_enum_def(ti_task_t * task, ti_member_t * member);
int ti_task_add_mod_enum_del(
        ti_task_t * task,
        ti_enum_t * enum_,
        ti_name_t * name);
int ti_task_add_mod_enum_mod(
        ti_task_t * task,
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val);
int ti_task_add_mod_enum_ren(
        ti_task_t * task,
        ti_enum_t * enum_,
        ti_name_t * oldname,
        ti_name_t * newname);
int ti_task_add_del_enum(ti_task_t * task, ti_enum_t * enum_);
int ti_task_add_whitelist_add(
        ti_task_t * task,
        ti_user_t * user,
        ti_val_t * val,
        int wid);
int ti_task_add_whitelist_del(
        ti_task_t * task,
        ti_user_t * user,
        ti_val_t * val,
        int wid);
void ti_task_pack_mig_add(msgpack_packer * pk, ti_mig_t * mig);

static inline size_t ti_task_size_mig_add(ti_mig_t * mig)
{
    return mig ? (48 + mig->query->n + mig->info->n + mig->by->n) : 0;
}


#endif /* TI_TASK_H_ */
