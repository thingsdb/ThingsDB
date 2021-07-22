/*
 * ti/room.c
 */

#include <ti/room.h>
#include <ti/proto.t.h>
#include <ex.h>

ti_room_t * ti_room_create(uint64_t id, ti_collection_t * collection)
{
    ti_room_t * room = malloc(sizeof(ti_room_t));
    if (!room)
        return NULL;

    room->id = id;
    room->collection = collection;
    room->listeners = vec_new(2);

    if (!room->listeners)
    {
        free(room);
        return NULL;
    }

    return room;
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

    for (vec_each(room->listeners, ti_watch_t, watch))
    {
        if (ti_stream_is_closed(watch->stream))
            continue;

        if (ti_stream_write_rpkg(watch->stream, rpkg))
            log_critical(EX_INTERNAL_S);
    }

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

        msgpack_pack_map(pk, 1);
        mp_pack_str(pk, "id");
        msgpack_pack_uint64(pk, room->id);

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_DELETE, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
}

void ti_room_emit_event(ti_room_t * room, const char * data, size_t sz)
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
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_EVENT, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
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

        mp_pack_str(&pk, status);

        pkg = (ti_pkg_t *) buffer.data;
        pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_NODE_STATUS, buffer.size);

        room__write_pkg(room, pkg);  /* destroys `pkg` */
    }
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

    if (msgpack_pack_map(pk, 1) ||
        mp_pack_str(pk, "id") ||
        msgpack_pack_uint64(pk, room->id))
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
    if (!ti_is_shutting_down())
        /*
         * Do not emit the deletion of things when the reason for the deletion
         * is "shutting down" of the node.
         */
        room__emit_delete(room);

    vec_destroy(room->listeners, (vec_destroy_cb) ti_watch_drop);
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

    for (vec_each(room->listeners, ti_watch_t, watch))
    {
        if (watch->stream == stream)
            return 0;

        if (!watch->stream)
        {
            watch->stream = stream;
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
