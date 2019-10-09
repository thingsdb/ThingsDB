/*
 * ti/task.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/task.h>
#include <ti/ncache.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/field.h>
#include <ti/data.h>
#include <util/mpack.h>
#include <util/cryptx.h>

static inline void task__upd_approx_sz(ti_task_t * task, ti_data_t * data)
{
    task->approx_sz += data->n;
}

ti_task_t * ti_task_create(uint64_t event_id, ti_thing_t * thing)
{
    ti_task_t * task = malloc(sizeof(ti_task_t));
    if (!task)
        return NULL;

    task->event_id = event_id;
    task->thing = ti_grab(thing);
    task->jobs = vec_new(1);
    task->approx_sz = 11;  /* thing_id (9) + map (1) + [close map] (1) */
    if (!task->jobs)
    {
        ti_task_destroy(task);
        return NULL;
    }

    return task;
}

ti_task_t * ti_task_get_task(ti_event_t * ev, ti_thing_t * thing, ex_t * e)
{
    ti_task_t * task = vec_last(ev->_tasks);
    if (task && task->thing == thing)
        return task;

    task = ti_task_create(ev->id, thing);
    if (!task)
        goto failed;

    if (vec_push(&ev->_tasks, task))
        goto failed;

    return task;

failed:
    ti_task_destroy(task);
    ex_set_mem(e);
    return NULL;
}

void ti_task_destroy(ti_task_t * task)
{
    if (!task)
        return;
    vec_destroy(task->jobs, free);
    ti_val_drop((ti_val_t *) task->thing);
    free(task);
}

ti_pkg_t * ti_task_pkg_watch(ti_task_t * task)
{
    ti_pkg_t * pkg;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, task->approx_sz, sizeof(ti_pkg_t)))
        return NULL;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 3);

    mp_pack_strn(&pk, TI_KIND_S_THING, 1);
    msgpack_pack_uint64(&pk, task->thing->id);

    mp_pack_str(&pk, "event");
    msgpack_pack_uint64(&pk, task->event_id);

    mp_pack_str(&pk, "jobs");
    msgpack_pack_array(&pk, task->jobs->n);
    for (vec_each(task->jobs, ti_data_t, data))
    {
        mp_pack_append(&pk, data->data, data->n);
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_CLIENT_WATCH_UPD, buffer.size);

    return pkg;
}

int ti_task_add_add(ti_task_t * task, ti_name_t * name, vec_t * added)
{
    assert (added->n);
    assert (name);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "add") ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, name->str, name->n) ||
        msgpack_pack_array(&pk, added->n)
    ) goto fail_pack;

    for (vec_each(added, ti_thing_t, thing))
        if ((!thing->id && ti_thing_gen_id(thing)) ||
            ti_val_to_pk((ti_val_t *) thing, &pk, TI_VAL_PACK_TASK)
        ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

int ti_task_add_set(ti_task_t * task, ti_name_t * name, ti_val_t * val)
{
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_data_t), sizeof(ti_data_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_val_gen_ids(val))
        return -1;

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "set") ||
        msgpack_pack_map(&pk, 1)
    ) goto fail_pack;

    if (mp_pack_strn(&pk, name->str, name->n) ||
        ti_val_to_pk(val, &pk, TI_VAL_PACK_TASK)
    ) goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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
    size_t alloc = ti_type_approx_pack_sz(type);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_type");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, type->name, type->name_n);

    mp_pack_str(&pk, "fields");
    ti_type_fields_to_pk(type, &pk);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del");
    mp_pack_strn(&pk, rname->data, rname->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "del_collection");
    msgpack_pack_uint64(&pk, collection_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del_expired");
    msgpack_pack_uint64(&pk, after_ts);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_del_procedure(ti_task_t * task, ti_raw_t * name)
{
    size_t alloc = 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del_procedure");
    mp_pack_strn(&pk, name->data, name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del_token");
    mp_pack_strn(&pk, key, sizeof(ti_token_key_t));

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "del_type");
    msgpack_pack_uint16(&pk, type->type_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "del_user");
    msgpack_pack_uint64(&pk, user->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "grant");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, scope_id);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, mask);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_collection");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, collection->name->data, collection->name->n);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "root");
    msgpack_pack_uint64(&pk, collection->root->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_node");
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

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_new_procedure(ti_task_t * task, ti_procedure_t * procedure)
{
    ti_ncache_t * ncache = procedure->closure->node->data;
    size_t alloc = strlen(ncache->query) + procedure->name->n + 64;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_procedure");
    msgpack_pack_map(&pk, 1);

    mp_pack_strn(&pk, procedure->name->data, procedure->name->n);
    if (ti_closure_to_pk(procedure->closure, &pk))
        goto fail_pack;

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_token");
    msgpack_pack_map(&pk, 4);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "key");
    mp_pack_strn(&pk, token->key, sizeof(ti_token_key_t));

    mp_pack_str(&pk, "expire_ts");
    msgpack_pack_uint64(&pk, token->expire_ts);

    mp_pack_str(&pk, "description");
    mp_pack_str(&pk, token->description);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "new_user");
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "username");
    mp_pack_strn(&pk, user->name->data, user->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_add(
        ti_task_t * task,
        ti_type_t * type,
        ti_val_t * val)
{
    ti_field_t * field = vec_last(type->fields);
    size_t alloc = val ? 8192 : 64 + field->name->n + field->spec_raw->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "mod_type_add");
    msgpack_pack_map(&pk, val ? 4 : 3);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, field->name->str, field->name->n);

    mp_pack_str(&pk, "spec");
    mp_pack_strn(&pk, field->spec_raw->data, field->spec_raw->n);

    if (val)
    {
        mp_pack_str(&pk, "init");
        if (ti_val_to_pk(val, &pk, TI_VAL_PACK_TASK))
            goto fail_pack;
    }

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "mod_type_del");
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, name->str, name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_mod_type_mod(ti_task_t * task, ti_field_t * field)
{
    size_t alloc = 64 + field->name->n + field->spec_raw->n;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "mod_type_mod");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "type_id");
    msgpack_pack_uint16(&pk, field->type->type_id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, field->name->str, field->name->n);

    mp_pack_str(&pk, "spec");
    mp_pack_strn(&pk, field->spec_raw->data, field->spec_raw->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "del_node");
    msgpack_pack_uint32(&pk, node_id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_remove(ti_task_t * task, ti_name_t * name, vec_t * removed)
{
    assert (removed->n);
    assert (name);
    size_t alloc = 64 + name->n + removed->n * 9;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "remove");
    msgpack_pack_map(&pk, 1);

    mp_pack_strn(&pk, name->str, name->n);
    msgpack_pack_array(&pk, removed->n);

    for (vec_each(removed, ti_thing_t, thing))
        msgpack_pack_uint64(&pk, thing->id);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "rename_collection");
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, collection->root->id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, collection->name->data, collection->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "rename_user");
    msgpack_pack_map(&pk, 2);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "name");
    mp_pack_strn(&pk, user->name->data, user->name->n);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "revoke");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "scope");
    msgpack_pack_uint64(&pk, scope_id);

    mp_pack_str(&pk, "user");
    msgpack_pack_uint64(&pk, user->id);

    mp_pack_str(&pk, "mask");
    msgpack_pack_uint64(&pk, mask);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "set_password");
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

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_set_quota(
        ti_task_t * task,
        uint64_t collection_id,
        ti_quota_enum_t quota_tp,
        size_t quota)
{
    size_t alloc = 128;
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    mp_pack_str(&pk, "set_quota");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "collection");
    msgpack_pack_uint64(&pk, collection_id);

    mp_pack_str(&pk, "quota_tp");
    msgpack_pack_uint8(&pk, quota_tp);

    mp_pack_str(&pk, "quota");
    msgpack_pack_uint64(&pk, quota);

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_splice(
        ti_task_t * task,
        ti_name_t * name,
        ti_varr_t * varr,
        uint32_t i,  /* start index */
        uint32_t c,  /* number of items to remove */
        uint32_t n)  /* number of items to add */
{
    assert (!varr || varr->tp == TI_VAL_ARR);
    assert ((n && varr) || !n);
    assert (name);
    ti_val_t * val;
    size_t alloc = n ? 8192 : (name->n + 64);
    ti_data_t * data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_data_t)))
        return -1;
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);
    mp_pack_str(&pk, "splice");

    msgpack_pack_map(&pk, 1);
    mp_pack_strn(&pk, name->str, name->n);
    msgpack_pack_array(&pk, 2 + n);
    msgpack_pack_uint32(&pk, i);
    msgpack_pack_uint32(&pk, c);

    for (c = i + n; i < c; ++i)
    {
        val = vec_get(varr->vec, i);

        if (ti_val_gen_ids(val) ||
            ti_val_to_pk(
                vec_get(varr->vec, i),
                &pk,
                TI_VAL_PACK_TASK))
            goto fail_pack;
    }

    data = (ti_data_t *) buffer.data;
    ti_data_init(data, buffer.size);

    if (vec_push(&task->jobs, data))
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

