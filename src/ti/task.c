/*
 * ti/task.c
 */
#include <assert.h>
#include <qpack.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/task.h>
#include <ti/ncache.h>
#include <ti/proto.h>
#include <ti/raw.h>
#include <ti/field.h>
#include <ti/data.h>
#include <util/qpx.h>
#include <util/cryptx.h>

/* negative value is used for packing tasks */
const int pack_task = -1;

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
    qp_packer_t * packer = qpx_packer_create(task->approx_sz, 2);
    if (!packer)
        return NULL;
    (void) qp_add_map(&packer);
    (void) qp_add_raw(packer, (const uchar *) TI_KIND_S_THING, 1);
    (void) qp_add_int(packer, task->thing->id);
    (void) qp_add_raw_from_str(packer, "event");
    (void) qp_add_int(packer, task->event_id);
    (void) qp_add_raw_from_str(packer, "jobs");
    (void) qp_add_array(&packer);
    for (vec_each(task->jobs, ti_data_t, data))
    {
        (void) qp_add_qp(packer, data->data, data->n);
    }
    (void) qp_close_array(packer);
    (void) qp_close_map(packer);

    pkg = qpx_packer_pkg(packer, TI_PROTO_CLIENT_WATCH_UPD);

    qpx_log("generated task for subscribers:",
            pkg->data, pkg->n, LOGGER_DEBUG);

    return pkg;
}

int ti_task_add_add(ti_task_t * task, ti_name_t * name, vec_t * added)
{
    assert (added->n);
    assert (name);
    ti_data_t * data;
    qp_packer_t * packer = qp_packer_create2(1024, 8);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "add");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n) ||
        qp_add_array(&packer) ||
        qp_add_int(packer, added->n))
        goto fail_packer;

    for (vec_each(added, ti_thing_t, thing))
        if ((!thing->id && ti_thing_gen_id(thing)) ||
            ti_val_to_packer((ti_val_t *) thing, &packer, pack_task))
            goto fail_packer;

    if (qp_close_array(packer) || qp_close_map(packer) || qp_close_map(packer))
        goto fail_packer;

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_packer:
    qp_packer_destroy(packer);
    return -1;
}

int ti_task_add_set(ti_task_t * task, ti_name_t * name, ti_val_t * val)
{
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(512, 8);

    if (!packer)
        return -1;

    if (ti_val_gen_ids(val))
        goto fail_packer;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n))
        goto fail_packer;

    if (ti_val_to_packer(val, &packer, pack_task))
        goto fail_packer;

    if (qp_close_map(packer) || qp_close_map(packer))
        goto fail_packer;

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_packer:
    qp_packer_destroy(packer);
    return -1;
}

int ti_task_add_define(ti_task_t * task, ti_type_t * type)
{
    ti_data_t * data;
    size_t alloc_sz = ti_type_approx_pack_sz(type);
    qp_packer_t * packer = ti_data_packer(alloc_sz, 3);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "define");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "type_id");
    (void) qp_add_int(packer, type->type_id);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, (const uchar *) type->name, type->name_n);
    (void) qp_add_raw_from_str(packer, "fields");
    (void) qp_add_map(&packer);
    for (vec_each(type->fields, ti_field_t, field))
    {
        (void) qp_add_raw(
                packer,
                (const uchar *) field->name->str,
                field->name->n);
        (void) qp_add_raw(packer, field->spec_raw->data, field->spec_raw->n);
    }
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(20 + rname->n, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del");
    (void) qp_add_raw(packer, rname->data, rname->n);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(26, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_collection");
    (void) qp_add_int(packer, collection_id);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(24, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_expired");
    (void) qp_add_int(packer, after_ts);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(24 + name->n, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_procedure");
    (void) qp_add_raw(packer, name->data, name->n);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(sizeof(ti_token_key_t) + 16, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_token");
    (void) qp_add_raw(packer, (const uchar *) key, sizeof(ti_token_key_t));
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(24, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "del_user");
    (void) qp_add_int(packer, user->id);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(64, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "grant");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "scope");
    (void) qp_add_int(packer, scope_id);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int(packer, mask);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(64 + collection->name->n, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_collection");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, collection->name->data, collection->name->n);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "root");
    (void) qp_add_int(packer, collection->root->id);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(256, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_node");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, node->id);
    (void) qp_add_raw_from_str(packer, "port");
    (void) qp_add_int(packer, node->port);
    (void) qp_add_raw_from_str(packer, "addr");
    (void) qp_add_raw_from_str(packer, node->addr);
    (void) qp_add_raw_from_str(packer, "secret");
    (void) qp_add_raw(packer, (uchar *) node->secret, CRYPTX_SZ);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    ti_ncache_t * ncache = procedure->closure->node->data;
    size_t alloc_sz = strlen(ncache->query) + procedure->name->n + 32;
    qp_packer_t * packer = ti_data_packer(alloc_sz, 3);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_procedure");
    (void) qp_add_map(&packer);
    (void) qp_add_raw(packer, procedure->name->data, procedure->name->n);
    if (ti_closure_to_packer(procedure->closure, &packer))
        goto fail_packer;
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_packer:
    qp_packer_destroy(packer);
    return -1;
}

int ti_task_add_new_token(
        ti_task_t * task,
        ti_user_t * user,
        ti_token_t * token)
{
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(256, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_token");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "key");
    (void) qp_add_raw(packer,
            (const uchar *) token->key,
            sizeof(ti_token_key_t));
    (void) qp_add_raw_from_str(packer, "expire_ts");
    (void) qp_add_int(packer, token->expire_ts);
    (void) qp_add_raw_from_str(packer, "description");
    (void) qp_add_raw_from_str(packer, token->description);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(40 + user->name->n, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "new_user");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "username");
    (void) qp_add_raw(packer, user->name->data, user->name->n);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_pop_node(ti_task_t * task, uint8_t node_id)
{
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(24, 1);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "pop_node");
    (void) qp_add_int(packer, node_id);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(32 + name->n + removed->n * 9, 3);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "remove");
    (void) qp_add_map(&packer);

    (void) qp_add_raw(packer, (const uchar *) name->str, name->n);
    (void) qp_add_array(&packer);
    (void) qp_add_int(packer, removed->n);

    for (vec_each(removed, ti_thing_t, thing))
        (void) qp_add_int(packer, thing->id);

    (void) qp_close_array(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    size_t qpsize = 50 + collection->name->n;
    qp_packer_t * packer = ti_data_packer(qpsize, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "rename_collection");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, collection->root->id);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, collection->name->data, collection->name->n);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(50 + user->name->n, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "rename_user");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "name");
    (void) qp_add_raw(packer, user->name->data, user->name->n);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;
}

int ti_task_add_replace_node(ti_task_t * task, ti_node_t * node)
{
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(256, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "replace_node");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, node->id);
    (void) qp_add_raw_from_str(packer, "port");
    (void) qp_add_int(packer, node->port);
    (void) qp_add_raw_from_str(packer, "addr");
    (void) qp_add_raw_from_str(packer, node->addr);
    (void) qp_add_raw_from_str(packer, "secret");
    (void) qp_add_raw(packer, (uchar *) node->secret, CRYPTX_SZ);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(64, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "revoke");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "scope");
    (void) qp_add_int(packer, scope_id);
    (void) qp_add_raw_from_str(packer, "user");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "mask");
    (void) qp_add_int(packer, mask);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(40 + CRYPTX_SZ, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set_password");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "id");
    (void) qp_add_int(packer, user->id);
    (void) qp_add_raw_from_str(packer, "password");
    if (user->encpass)
        (void) qp_add_raw_from_str(packer, user->encpass);
    else
        (void) qp_add_null(packer);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
    ti_data_t * data;
    qp_packer_t * packer = ti_data_packer(128, 2);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "set_quota");
    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "collection");
    (void) qp_add_int(packer, collection_id);
    (void) qp_add_raw_from_str(packer, "quota_tp");
    (void) qp_add_int(packer, quota_tp);
    (void) qp_add_raw_from_str(packer, "quota");
    (void) qp_add_int(packer, quota);
    (void) qp_close_map(packer);
    (void) qp_close_map(packer);

    data = ti_data_from_packer(packer);

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
        int64_t i,  /* start index */
        int64_t c,  /* number of items to remove */
        int32_t n)  /* number of items to add */
{
    assert (!varr || varr->tp == TI_VAL_ARR);
    assert (name);
    ti_data_t * data;
    qp_packer_t * packer = n
            ? ti_data_packer(1024, 8)
            : ti_data_packer(name->n + 64, 3);

    if (!packer)
        return -1;

    (void) qp_add_map(&packer);
    (void) qp_add_raw_from_str(packer, "splice");
    (void) qp_add_map(&packer);

    if (qp_add_raw(packer, (const uchar *) name->str, name->n) ||
        qp_add_array(&packer) ||
        qp_add_int(packer, i) ||
        qp_add_int(packer, c) ||
        qp_add_int(packer, n))
        goto fail_packer;

    if (varr)
    {
        if (ti_val_gen_ids((ti_val_t *) varr))
            goto fail_packer;

        for (c = i + n; i < c; ++i)
            if (ti_val_to_packer(vec_get(varr->vec, i), &packer, pack_task))
                goto fail_packer;
    }

    if (qp_close_array(packer) || qp_close_map(packer) || qp_close_map(packer))
        goto fail_packer;

    data = ti_data_from_packer(packer);

    if (vec_push(&task->jobs, data))
        goto fail_data;

    task__upd_approx_sz(task, data);
    return 0;

fail_data:
    free(data);
    return -1;

fail_packer:
    qp_packer_destroy(packer);
    return -1;
}

