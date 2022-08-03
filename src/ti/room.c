/*
 * ti/room.c
 */
#include <ti.h>
#include <ti/collection.inline.h>
#include <ti/pkg.t.h>
#include <ti/proto.t.h>
#include <ti/room.h>
#include <ti/room.inline.h>
#include <ti/room.t.h>
#include <ti/rpkg.h>
#include <ti/rpkg.t.h>
#include <ti/stream.h>
#include <ti/stream.t.h>
#include <ti/val.inline.h>
#include <ti/watch.h>
#include <ti/watch.t.h>
#include <util/vec.h>
#include <ex.h>

ti_room_t * ti_room_create(uint64_t id, ti_collection_t * collection)
{
    ti_room_t * room = malloc(sizeof(ti_room_t));
    if (!room)
        return NULL;

    room->ref = 1;
    room->tp = TI_VAL_ROOM;
    room->id = id;
    room->collection = collection;
    room->listeners = vec_new(2);

    if (!room->listeners)
        return free(room), NULL;

    return room;
}

static void room__write_rpkg(ti_room_t * room, ti_rpkg_t * rpkg)
{
    for (vec_each(room->listeners, ti_watch_t, watch))
    {
        if (ti_stream_is_closed(watch->stream))
            continue;

        if (ti_stream_write_rpkg(watch->stream, rpkg))
            log_critical(EX_INTERNAL_S);
    }
}

/*
 * This function destroys `pkg`. Thus, do not use or free `pkg` after calling
 * this function.
 */
static void room__write_pkg(ti_room_t * room, ti_pkg_t * pkg)
{
    ti_rpkg_t * rpkg = ti_rpkg_create(pkg);
    if (!rpkg)
    {
        free(pkg);
        log_critical(EX_MEMORY_S);
        return;
    }
    room__write_rpkg(room, rpkg);
    ti_rpkg_drop(rpkg);
}

/*
 * Emit room delete to all listeners and destroy the listeners vector.
 */
static void room__emit_delete(ti_room_t * room)
{
    if (ti_room_has_listeners(room))
    {
        msgpack_packer pk;
        msgpack_sbuffer buffer;
        ti_pkg_t * pkg;

        if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
        {
            log_critical(EX_MEMORY_S);
            return;
        }

        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

        msgpack_pack_map(&pk, 1);
        mp_pack_str(&pk, "id");
        msgpack_pack_uint64(&pk, room->id);

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_DELETE, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
}

void ti_room_emit_data(ti_room_t * room, const void * data, size_t sz)
{
    if (ti_room_has_listeners(room))
    {
        size_t alloc = sizeof(ti_pkg_t) + sz;
        msgpack_packer pk;
        msgpack_sbuffer buffer;
        ti_pkg_t * pkg;

        if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_pkg_t)))
        {
            log_critical(EX_MEMORY_S);
            return;
        }

        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

        mp_pack_append(&pk, data, sz);

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_EMIT, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
}

int ti_room_emit(
        ti_room_t * room,
        ti_query_t * query,
        vec_t * args,
        const char * event,
        size_t event_n,
        int deep,
        int flags)
{
    size_t sz, alloc = 8192;
    msgpack_sbuffer buffer;
    ti_pkg_t * node_pkg, * client_pkg;
    ti_rpkg_t * node_rpkg, * client_rpkg;
    ti_vp_t vp = {
            .query=query,  /* required for wrap type, may be NULL from API */
    };

    if (mp_sbuffer_alloc_init(&buffer, alloc, sizeof(ti_pkg_t)))
        return -1;

    msgpack_packer_init(&vp.pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_array(&vp.pk, 3);
    msgpack_pack_uint64(&vp.pk, room->collection->root->id);
    msgpack_pack_uint64(&vp.pk, room->id);

    sz = buffer.size;

    msgpack_pack_map(&vp.pk, 3);

    mp_pack_str(&vp.pk, "id");
    msgpack_pack_uint64(&vp.pk, room->id);

    mp_pack_str(&vp.pk, "event");
    mp_pack_strn(&vp.pk, event, event_n);

    mp_pack_str(&vp.pk, "args");
    msgpack_pack_array(&vp.pk, args ? args->n : 0);

    if (args)
        for (vec_each(args, ti_val_t, val))
            if (ti_val_to_client_pk(val, &vp, deep, flags))
                goto fail_pack;

    node_pkg = (ti_pkg_t *) buffer.data;
    node_rpkg = ti_rpkg_create(node_pkg);
    pkg_init(node_pkg, TI_PROTO_EV_ID, TI_PROTO_NODE_ROOM_EMIT, buffer.size);

    /* Bug #214, use buffer.data only when finished writing */
    client_pkg = ti_pkg_new(
            TI_PROTO_EV_ID,
            TI_PROTO_CLIENT_ROOM_EMIT,
            buffer.data + sz,
            buffer.size - sz);
    client_rpkg = ti_rpkg_create(client_pkg);

    if (!client_pkg || !node_rpkg || !client_rpkg)
        goto fail_pkg;

    ti_nodes_write_rpkg(node_rpkg);
    ti_rpkg_drop(node_rpkg);

    room__write_rpkg(room, client_rpkg);
    ti_rpkg_drop(client_rpkg);

    return 0;

fail_pkg:
    /* may leak a few bytes */
    free(client_pkg);
    free(node_pkg);
    return -1;

fail_pack:
    msgpack_sbuffer_destroy(&buffer);
    return -1;
}

int ti_room_emit_from_pkg(
        ti_collection_t * collection,
        ti_pkg_t * pkg,
        ex_t * e)
{
    size_t nargs;
    mp_unp_t up;
    mp_obj_t obj, mp_id, mp_event;
    ti_room_t * room;
    vec_t * args;
    ti_vup_t vup = {
            .isclient = true,
            .collection = collection,
            .up = &up,
    };

    mp_unp_init(&up, pkg->data, pkg->n);

    mp_next(&up, &obj);     /* array with at least size 1 */
    mp_skip(&up);           /* scope */

    if (obj.via.sz < 3 ||
        mp_next(&up, &mp_id) != MP_U64 ||
        mp_next(&up, &mp_event) != MP_STR)
    {
        ex_set(e, EX_BAD_DATA, "invalid emit request");
        return e->nr;
    }

    nargs = obj.via.sz - 3;

    if (nargs)
    {
        args = vec_new(nargs);
        if (!args)
            return ex_set_mem(e), e->nr;
    }
    else
        args = NULL;

    uv_mutex_lock(collection->lock);

    room = ti_collection_room_by_id(collection, mp_id.via.u64);
    if (room)
    {
        while (nargs--)
        {
            ti_val_t * val = ti_val_from_vup_e(&vup, e);
            if (!val)
                break;

            VEC_push(args, val);
        }
    }
    else
        ex_set(e, EX_LOOKUP_ERROR,
                "collection `%.*s` has no `room` with id %"PRIu64,
                collection->name->n,
                (char *) collection->name->data,
                mp_id.via.u64);

    uv_mutex_unlock(collection->lock);

    if (e->nr)
        goto fail0;

    if (ti_room_emit(
            room,
            NULL,
            args,
            mp_event.via.str.data,
            mp_event.via.str.n,
            TI_MAX_DEEP,
            0))
        ex_set_mem(e);

fail0:
    vec_destroy(args, (vec_destroy_cb) ti_val_unsafe_drop);
    return e->nr;
}

void ti_room_emit_node_status(ti_room_t * room, const char * status)
{
    if (ti_room_has_listeners(room))
    {
        msgpack_packer pk;
        msgpack_sbuffer buffer;
        ti_pkg_t * pkg;


        if (mp_sbuffer_alloc_init(&buffer, 64, sizeof(ti_pkg_t)))
        {
            log_critical(EX_MEMORY_S);
            return;
        }

        msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

        msgpack_pack_map(&pk, 2);

        mp_pack_str(&pk, "id");
        msgpack_pack_uint32(&pk, ti.node->id);

        mp_pack_str(&pk, "status");
        mp_pack_str(&pk, status);

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_NODE_STATUS, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
}

typedef struct
{
    uint64_t room_id;
    ti_stream_t * stream;
} room__async_t;

/*
 * Emit room leave to a given listener.
 */
static void room__async_emit_join_cb(uv_async_t * task)
{
    room__async_t * w = task->data;
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;

    if (ti_stream_is_closed(w->stream))
        goto done;

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        goto done;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "id") ||
        msgpack_pack_uint64(&pk, w->room_id))
    {
        log_critical(EX_MEMORY_S);
        goto done;
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_JOIN, buffer.size);

    if (ti_stream_write_pkg(w->stream, pkg))
        log_critical(EX_INTERNAL_S);

done:
    ti_stream_drop(w->stream);
    free(w);
    uv_close((uv_handle_t *) task, (uv_close_cb) free);
}

static void room__async_emit_join(ti_room_t * room, ti_stream_t * stream)
{
    uv_async_t * task = malloc(sizeof(uv_async_t));
    room__async_t * w = malloc(sizeof(room__async_t));

    if (!task || !w)
        goto failed;

    w->stream = stream;
    w->room_id = room->id;
    task->data = w;

    if (uv_async_init(ti.loop, task, (uv_async_cb) room__async_emit_join_cb) ||
        uv_async_send(task))
        goto failed;

    ti_incref(stream);

    return;  /* success */

failed:
    log_critical(EX_INTERNAL_S);
    free(task);
    free(w);
}

/*
 * Emit room leave to a given listener.
 */
static void room__emit_leave(ti_room_t * room, ti_stream_t * stream)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;

    if (ti_stream_is_closed(stream))
        return;

    if (mp_sbuffer_alloc_init(&buffer, 32, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_map(&pk, 1) ||
        mp_pack_str(&pk, "id") ||
        msgpack_pack_uint64(&pk, room->id))
    {
        log_critical(EX_MEMORY_S);
        return;
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_LEAVE, buffer.size);

    if (ti_stream_write_pkg(stream, pkg))
        log_critical(EX_INTERNAL_S);
}

void ti_room_destroy(ti_room_t * room)
{
    if (!ti_flag_test(TI_FLAG_SIGNAL))
        /*
         * Do not emit the deletion of things when the reason for the deletion
         * is "shutting down" of the node.
         */
        room__emit_delete(room);

    vec_destroy(room->listeners, (vec_destroy_cb) ti_watch_drop);

    if (room->id)
        (void) imap_pop(room->collection->rooms, room->id);

    free(room);
}

int ti_room_gen_id(ti_room_t * room)
{
    assert (!room->id);

    room->id = ti_next_free_id();
    return ti_room_to_map(room);
}

int ti_room_join(ti_room_t * room, ti_stream_t * stream)
{
    ti_watch_t * watch;

    for (vec_each(room->listeners, ti_watch_t, w))
    {
        if (w->stream == stream)
            return 0;

        if (!w->stream)
        {
            w->stream = stream;
            watch = w;
            goto finish;
        }
    }

    watch = ti_watch_create(stream);
    if (!watch)
        return -1;

    if (vec_push(&room->listeners, watch))
        goto failed;

finish:
    if (vec_push_create(&stream->listeners, watch))
        goto failed;

    if (room->id)
        room__async_emit_join(room, stream);

    return 0;

failed:
    /* when this fails, a few bytes might leak */
    watch->stream = NULL;
    return -1;
}

int ti_room_leave(ti_room_t * room, ti_stream_t * stream)
{
    size_t idx = 0;

    for (vec_each(room->listeners, ti_watch_t, watch), ++idx)
    {
        if (watch->stream == stream)
        {
            watch->stream = NULL;
            vec_swap_remove(room->listeners, idx);
            room__emit_leave(room, stream);
            return 0;
        }
    }
    return 0;
}

int ti_room_copy(ti_room_t ** roomaddr)
{
    ti_room_t * room = ti_room_create(0, (*roomaddr)->collection);
    if (!room)
        return -1;

    ti_val_unsafe_drop((ti_val_t *) *roomaddr);
    *roomaddr = room;
    return 0;
}
