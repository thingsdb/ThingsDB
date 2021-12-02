/*
 * ti/task.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/data.h>
#include <ti/enum.inline.h>
#include <ti/field.h>
#include <ti/method.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/task.h>
#include <ti/type.h>
#include <ti/val.inline.h>
#include <util/cryptx.h>
#include <util/mpack.h>

static inline void task__upd_approx_sz(ti_task_t * task, ti_data_t * data)
{
    task->approx_sz += data->n;
}

ti_task_t * ti_task_create(uint64_t change_id, ti_thing_t * thing)
{
    ti_task_t * task = malloc(sizeof(ti_task_t));
    if (!task)
        return NULL;

    task->change_id = change_id;
    task->thing = thing;
    task->list = vec_new(1);
    task->approx_sz = 11;  /* thing_id (9) + map (1) + [close map] (1) */

    ti_incref(thing);

    if (!task->list)
    {
        ti_task_destroy(task);
        return NULL;
    }

    return task;
}

ti_task_t * ti_task_get_task(ti_change_t * change, ti_thing_t * thing)
{
    ti_task_t * task = vec_last(change->_tasks);
    if (task && task->thing == thing)
        return task;

    task = ti_task_create(change->id, thing);
    if (!task)
        goto failed;

    if (vec_push(&change->_tasks, task))
        goto failed;

    return task;

failed:
    ti_task_destroy(task);
    return NULL;
}

void ti_task_destroy(ti_task_t * task)
{
    if (!task)
        return;
    vec_destroy(task->list, free);
    ti_val_unsafe_drop((ti_val_t *) task->thing);
    free(task);
}

int ti_task_add_set_add(ti_task_t * task, ti_raw_t * key, vec_t * added)
{
    assert (added->n);
    assert (key);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_array(&pk, 2) ||
        msgpack_pack_uint8(&pk, TI_TASK_SET_ADD) ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, key->data, key->n) ||
        msgpack_pack_array(&pk, added->n)
    ) goto fail_pack;

    for (vec_each(added, ti_thing_t, thing))
        if ((!thing->id && ti_thing_gen_id(thing)) ||
            ti_val_to_store_pk((ti_val_t *) thing, &pk)
        ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_thing_clear(ti_task_t * task)
{
    size_t alloc = 32;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint8(&pk, TI_TASK_THING_CLEAR);
    msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_arr_clear(ti_task_t * task, ti_raw_t * key)
{
    assert (key);
    size_t alloc = 32;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint8(&pk, TI_TASK_ARR_CLEAR);
    mp_pack_strn(&pk, key->data, key->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_clear(ti_task_t * task, ti_raw_t * key)
{
    assert (key);
    size_t alloc = 32;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint8(&pk, TI_TASK_SET_CLEAR);
    mp_pack_strn(&pk, key->data, key->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set(ti_task_t * task, ti_raw_t * key, ti_val_t * val)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_val_gen_ids(val))
        return -1;

    if (msgpack_pack_array(&pk, 2) ||
        msgpack_pack_uint8(&pk, TI_TASK_SET) ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, key->data, key->n) ||
        ti_val_to_store_pk(val, &pk)
    ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_new_type(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 64 + type->rname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_TYPE);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, type->created_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, type->rname->data, type->rname->n);

    mp_pack_str(&pk, "wrap_only");
    mp_pack_bool(&pk, ti_type_is_wrap_only(type));

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_to_type(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 32;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_TO_TYPE);
    msgpack_pack_uint16(&pk, type->type_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_restrict(ti_task_t * task, uint16_t spec)
{
    size_t alloc = 32;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RESTRICT);
    msgpack_pack_uint16(&pk, spec);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}


int ti_task_add_set_type(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 64 + ti_type_fields_approx_pack_sz(type);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_TYPE);
    msgpack_pack_map(&pk, 5);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "wrap_only");
    mp_pack_bool(&pk, ti_type_is_wrap_only(type));

    mp_pack_str(&pk, "fields");
    ti_type_fields_to_pk(type, &pk);

    mp_pack_str(&pk, "methods");
    ti_type_methods_to_pk(type, &pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del(ti_task_t * task, ti_raw_t * rname)
{
    size_t alloc = 64 + rname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL);

    mp_pack_strn(&pk, rname->data, rname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_collection(ti_task_t * task, uint64_t collection_id)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_COLLECTION);
    msgpack_pack_uint64(&pk, collection_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_expired(ti_task_t * task, uint64_t after_ts)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_EXPIRED);
    msgpack_pack_uint64(&pk, after_ts);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_procedure(ti_task_t * task, ti_raw_t * name)
{
    size_t alloc = 64 + name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_PROCEDURE);
    mp_pack_strn(&pk, name->data, name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_token(ti_task_t * task, ti_token_key_t * key)
{
    size_t alloc = 128;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_TOKEN);
    mp_pack_strn(&pk, key, sizeof(ti_token_key_t));

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_type(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_TYPE);
    msgpack_pack_uint16(&pk, type->type_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_type(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 64 + type->rname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_TYPE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, type->rname->data, type->rname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_enum(ti_task_t * task, ti_enum_t * enum_)
{
    size_t alloc = 64 + enum_->rname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_ENUM);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, enum_->enum_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, enum_->rname->data, enum_->rname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_user(ti_task_t * task, ti_user_t * user)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_USER);
    msgpack_pack_uint64(&pk, user->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_module(ti_task_t * task, ti_module_t * module)
{
    size_t alloc = 64 + module->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_MODULE);
    mp_pack_strn(&pk, module->name->str, module->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_deploy_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * mdata)  /* mdata may be NULL */
{
    size_t alloc = 64 + module->name->n + (mdata ? mdata->n : 0);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEPLOY_MODULE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, module->name->str, module->name->n);

    mp_pack_str(&pk, "data");
    if (mdata)
        mp_pack_bin(&pk, mdata->data, mdata->n);
    else
        msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_module_scope(ti_task_t * task, ti_module_t * module)
{
    size_t alloc = 64 + module->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_MODULE_SCOPE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, module->name->str, module->name->n);

    mp_pack_str(&pk, "scope_id");
    if (module->scope_id)
        msgpack_pack_uint64(&pk, *module->scope_id);
    else
        msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_module_conf(ti_task_t * task, ti_module_t * module)
{
    size_t alloc = 64 + module->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_MODULE_CONF);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, module->name->str, module->name->n);

    mp_pack_str(&pk, "conf_pkg");
    if (module->conf_pkg)
    {
        mp_pack_bin(
                &pk,
                module->conf_pkg,
                sizeof(ti_pkg_t) + module->conf_pkg->n);
    }
    else
        msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_grant(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_GRANT);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, scope_id);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, mask);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_collection(
        ti_task_t * task,
        ti_collection_t * collection,
        ti_user_t * user)
{
    size_t alloc = 64 + collection->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_COLLECTION);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, collection->name->data, collection->name->n);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "root");
    msgpack_pack_uint64(&pk, collection->root->id);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, collection->created_at);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * source)
{
    size_t alloc = \
            128 + \
            module->name->n + \
            source->n + \
            (module->conf_pkg ? module->conf_pkg->n : 0);

    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_MODULE);
    msgpack_pack_map(&pk, 5);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, module->name->str, module->name->n);

    mp_pack_str(&pk, "source");
    mp_pack_strn(&pk, source->data, source->n);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, module->created_at);

    mp_pack_str(&pk, "conf_pkg");
    if (module->conf_pkg)
    {
        mp_pack_bin(
                &pk,
                module->conf_pkg,
                sizeof(ti_pkg_t) + module->conf_pkg->n);
    }
    else
        msgpack_pack_nil(&pk);

    mp_pack_str(&pk, "scope_id");
    if (module->scope_id)
        msgpack_pack_uint64(&pk, *module->scope_id);
    else
        msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_node(ti_task_t * task, ti_node_t * node)
{
    size_t alloc = 256;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_NODE);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint32(&pk, node->id);

    mp_pack_str(&pk, "port");
    msgpack_pack_uint16(&pk, node->port);

    mp_pack_str(&pk, "addr");
    mp_pack_str(&pk, node->addr);

    mp_pack_str(&pk, "secret");
    mp_pack_strn(&pk, node->secret, CRYPTX_SZ);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_procedure(ti_task_t * task, ti_procedure_t * procedure)
{
    size_t alloc = procedure->closure->node->len + procedure->name_n + 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_PROCEDURE);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, procedure->name, procedure->name_n);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, procedure->created_at);

    mp_pack_str(&pk, "closure");
    if (ti_closure_to_store_pk(procedure->closure, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_vtask_new(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 1024;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_NEW);
    msgpack_pack_map(&pk, 5);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    mp_pack_str(&pk, "run_at");
    msgpack_pack_uint64(&pk, vtask->run_at);

    mp_pack_str(&pk, "user_id");
    msgpack_pack_uint64(&pk, vtask->user->id);

    if (mp_pack_str(&pk, "closure") ||
        ti_closure_to_store_pk(vtask->closure, &pk) ||

        mp_pack_str(&pk, "args") ||
        msgpack_pack_array(&pk, vtask->args->n))
        goto fail_pack;

    /*
     * In the @thingsdb scope there are no things allowed as arguments so
     * the code below should run fine
     */
    for (vec_each(vtask->args, ti_val_t, val))
        if (ti_val_gen_ids(val) ||
            ti_val_to_store_pk(val, &pk))
            goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_vtask_del(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_DEL);
    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_vtask_cancel(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_CANCEL);
    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_vtask_finish(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 1024;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_FINISH);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    mp_pack_str(&pk, "run_at");
    msgpack_pack_uint64(&pk, vtask->run_at);

    mp_pack_str(&pk, "verr");
    if (vtask->verr->code == 0)
        msgpack_pack_nil(&pk);
    else if (ti_verror_to_store_pk(vtask->verr, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_vtask_set_args(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 1024;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_SET_ARGS);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    mp_pack_str(&pk, "args");
    msgpack_pack_array(&pk, vtask->args->n);

    /*
     * In the @thingsdb scope there are no things allowed as arguments so
     * the code below should run fine
     */
    for (vec_each(vtask->args, ti_val_t, val))
        if (ti_val_gen_ids(val) ||
            ti_val_to_store_pk(val, &pk))
            goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_vtask_set_owner(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 128;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_SET_OWNER);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    mp_pack_str(&pk, "owner");
    msgpack_pack_uint64(&pk, vtask->user->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_vtask_set_closure(ti_task_t * task, ti_vtask_t * vtask)
{
    size_t alloc = 64 + vtask->closure->node->len;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_VTASK_SET_CLOSURE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, vtask->id);

    mp_pack_str(&pk, "closure");
    if (ti_closure_to_store_pk(vtask->closure, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}
int ti_task_add_new_token(
        ti_task_t * task,
        ti_user_t * user,
        ti_token_t * token)
{
    size_t alloc = 256;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_TOKEN);
    msgpack_pack_map(&pk, 5);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "key");
    mp_pack_strn(&pk, token->key, sizeof(ti_token_key_t));

    mp_pack_str(&pk, "expire_ts");
    msgpack_pack_uint64(&pk, token->expire_ts);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, token->created_at);

    mp_pack_str(&pk, "description");
    mp_pack_str(&pk, token->description);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_user(ti_task_t * task, ti_user_t * user)
{
    size_t alloc = 64 + user->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_NEW_USER);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "username");
    mp_pack_strn(&pk, user->name->data, user->name->n);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, user->created_at);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_add_field(
        ti_task_t * task,
        ti_type_t * type,
        ti_val_t * dval)
{
    ti_field_t * field = VEC_last(type->fields);
    size_t alloc = dval ? 8192 : 64 + field->name->n + field->spec_raw->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_ADD);
    msgpack_pack_map(&pk, dval ? 5 : 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, field->name->str, field->name->n);

    mp_pack_str(&pk, "spec");
    mp_pack_strn(&pk, field->spec_raw->data, field->spec_raw->n);

    if (dval)
    {
        mp_pack_str(&pk, "dval");
        if (ti_val_to_store_pk(dval, &pk))
            goto fail_pack;
    }

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_mod_type_add_method(ti_task_t * task, ti_type_t * type)
{
    ti_method_t * method = VEC_last(type->methods);
    size_t alloc = 80 + method->closure->node->len + method->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_ADD);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, method->name->str, method->name->n);

    mp_pack_str(&pk, "closure");
    if (ti_closure_to_store_pk(method->closure, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_mod_type_rel_add(
        ti_task_t * task,
        ti_type_t * type1,
        ti_name_t * name1,
        ti_type_t * type2,
        ti_name_t * name2)
{
    size_t alloc = 96 + name1->n + name2->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_REL_ADD);
    msgpack_pack_map(&pk, 5);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type1->modified_at);

    mp_pack_str(&pk, "type1_id");
    msgpack_pack_uint16(&pk, type1->type_id);

    mp_pack_str(&pk, "name1");
    mp_pack_strn(&pk, name1->str, name1->n);

    mp_pack_str(&pk, "type2_id");
    msgpack_pack_uint16(&pk, type2->type_id);

    mp_pack_str(&pk, "name2");
    mp_pack_strn(&pk, name2->str, name2->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_rel_del(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * name)
{
    size_t alloc = 64 + name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_REL_DEL);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, name->str, name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_del(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * name)
{
    size_t alloc = 64 + name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_DEL);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, name->str, name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_mod_field(ti_task_t * task, ti_field_t * field)
{
    size_t alloc = 64 + field->name->n + field->spec_raw->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_MOD);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, field->type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, field->type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, field->name->str, field->name->n);

    mp_pack_str(&pk, "spec");
    mp_pack_strn(&pk, field->spec_raw->data, field->spec_raw->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_mod_method(
        ti_task_t * task,
        ti_type_t * type,
        ti_method_t * method)
{
    size_t alloc = 80 + method->name->n + method->closure->node->len;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_MOD);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, method->name->str, method->name->n);

    mp_pack_str(&pk, "closure");
    ti_closure_to_store_pk(method->closure, &pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_ren(
        ti_task_t * task,
        ti_type_t * type,
        ti_name_t * oldname,
        ti_name_t * newname)
{
    size_t alloc = 64 + oldname->n + newname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_REN);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, oldname->str, oldname->n);

    mp_pack_str(&pk, "to");
    mp_pack_strn(&pk, newname->str, newname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_wpo(ti_task_t * task, ti_type_t * type)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_TYPE_WPO);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, type->modified_at);

    mp_pack_str(&pk, "wrap_only");
    mp_pack_bool(&pk, ti_type_is_wrap_only(type));

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_node(ti_task_t * task, uint32_t node_id)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_NODE);
    msgpack_pack_uint32(&pk, node_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_remove(ti_task_t * task, ti_raw_t * key, vec_t * removed)
{
    assert (removed->n);
    assert (key);
    size_t alloc = 64 + key->n + removed->n * 9;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_REMOVE);
    msgpack_pack_map(&pk, 1);

    mp_pack_strn(&pk, key->data, key->n);
    msgpack_pack_array(&pk, removed->n);

    for (vec_each(removed, ti_thing_t, thing))
        msgpack_pack_uint64(&pk, thing->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_collection(
        ti_task_t * task,
        ti_collection_t * collection)
{
    size_t alloc = 64 + collection->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_COLLECTION);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, collection->root->id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, collection->name->data, collection->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_time_zone(ti_task_t * task, ti_collection_t * collection)
{
    size_t alloc = 128;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_TIME_ZONE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, collection->root->id);

    mp_pack_str(&pk, "tz");
    msgpack_pack_uint64(&pk, collection->tz->index);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_procedure(
        ti_task_t * task,
        ti_procedure_t * procedure,
        ti_raw_t * nname)
{
    size_t alloc = 64 + procedure->name_n + nname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_PROCEDURE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "old");
    mp_pack_strn(&pk, procedure->name, procedure->name_n);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, nname->data, nname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_module(
        ti_task_t * task,
        ti_module_t * module,
        ti_raw_t * nname)
{
    size_t alloc = 64 + module->name->n + nname->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_MODULE);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "old");
    mp_pack_strn(&pk, module->name->str, module->name->n);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, nname->data, nname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_rename_user(ti_task_t * task, ti_user_t * user)
{
    size_t alloc = 64 + user->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RENAME_USER);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, user->name->data, user->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_revoke(
        ti_task_t * task,
        uint64_t scope_id,
        ti_user_t * user,
        uint64_t mask)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_REVOKE);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, scope_id);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, mask);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_password(ti_task_t * task, ti_user_t * user)
{
    size_t alloc = 64 + CRYPTX_SZ;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_PASSWORD);
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "password");
    if (user->encpass)
        mp_pack_str(&pk, user->encpass);
    else
        msgpack_pack_nil(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_splice(
        ti_task_t * task,
        ti_raw_t * key,
        ti_varr_t * varr,
        uint32_t i,  /* start index */
        uint32_t c,  /* number of items to remove */
        uint32_t n)  /* number of items to add */
{
    assert (!varr || varr->tp == TI_VAL_ARR);
    assert ((n && varr) || !n);
    assert (key);
    ti_val_t * val;
    size_t alloc = n ? 8192 : (key->n + 64);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SPLICE);
    msgpack_pack_map(&pk, 1);

    mp_pack_strn(&pk, key->data, key->n);

    msgpack_pack_array(&pk, 2 + n);

    msgpack_pack_uint32(&pk, i);
    msgpack_pack_uint32(&pk, c);

    for (c = i + n; i < c; ++i)
    {
        val = VEC_get(varr->vec, i);

        if (ti_val_gen_ids(val) || ti_val_to_store_pk(val, &pk))
            goto fail_pack;
    }

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_arr_remove(ti_task_t * task, ti_raw_t * key, vec_t * vec)
{
    size_t alloc = 32 + key->n + (vec->n * 9);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_ARR_REMOVE);
    msgpack_pack_map(&pk, 1);

    mp_pack_strn(&pk, key->data, key->n);

    msgpack_pack_array(&pk, vec->n);

    for (vec_each_rev(vec, void, pos))
        msgpack_pack_uint32(&pk, (uintptr_t) pos);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_restore(ti_task_t * task)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_RESTORE);
    msgpack_pack_true(&pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_enum(ti_task_t * task, ti_enum_t * enum_)
{
    size_t alloc = 8192;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (enum_->enum_tp == TI_ENUM_THING)
        for(vec_each(enum_->members, ti_member_t, member))
            if (ti_val_gen_ids(member->val))
                return -1;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_SET_ENUM);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, enum_->enum_id);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, enum_->created_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, enum_->rname->data, enum_->rname->n);

    mp_pack_str(&pk, "members");
    if (ti_enum_members_to_store_pk(enum_, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_mod_enum_add(ti_task_t * task, ti_member_t * member)
{
    size_t alloc = 8192;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (ti_val_gen_ids(member->val))
        return -1;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_ENUM_ADD);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, member->enum_->enum_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, member->enum_->modified_at);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, member->name->str, member->name->n);

    mp_pack_str(&pk, "value");
    if (ti_val_to_store_pk(member->val, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_mod_enum_def(ti_task_t * task, ti_member_t * member)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_ENUM_DEF);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, member->enum_->enum_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, member->enum_->modified_at);

    mp_pack_str(&pk, "index");
    msgpack_pack_uint16(&pk, member->idx);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_enum_del(ti_task_t * task, ti_member_t * member)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_ENUM_DEL);
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, member->enum_->enum_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, member->enum_->modified_at);

    mp_pack_str(&pk, "index");
    msgpack_pack_uint16(&pk, member->idx);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_enum_mod(ti_task_t * task, ti_member_t * member)
{
    size_t alloc = 8192;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (ti_val_gen_ids(member->val))
        return -1;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_ENUM_MOD);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, member->enum_->enum_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, member->enum_->modified_at);

    mp_pack_str(&pk, "index");
    msgpack_pack_uint16(&pk, member->idx);

    mp_pack_str(&pk, "value");
    if (ti_val_to_store_pk(member->val, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_task_add_mod_enum_ren(ti_task_t * task, ti_member_t * member)
{
    size_t alloc = 96 + member->name->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_MOD_ENUM_REN);
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "enum_id");
    msgpack_pack_uint16(&pk, member->enum_->enum_id);

    mp_pack_str(&pk, "modified_at");
    msgpack_pack_uint64(&pk, member->enum_->modified_at);

    mp_pack_str(&pk, "index");
    msgpack_pack_uint16(&pk, member->idx);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, member->name->str, member->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_enum(ti_task_t * task, ti_enum_t * enum_)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);

    msgpack_pack_uint8(&pk, TI_TASK_DEL_ENUM);
    msgpack_pack_uint16(&pk, enum_->enum_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->list, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}
