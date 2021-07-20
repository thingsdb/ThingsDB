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
 * Emit room delete to all listeners and destroy the listeners vector.
 */
static void room__emit_delete(ti_room_t * room)
{
    msgpack_packer pk;
    msgpack_sbuffer buffer;
    ti_pkg_t * pkg;
    ti_rpkg_t * rpkg;

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
    pkg_init(pkg, TI_PROTO_EV_ID, TI_PROTO_CLIENT_ROOM_DELETE, buffer.size);

    rpkg = ti_rpkg_create(pkg);
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

        ti_watch_drop(watch);
    }

    free(room->listeners);
    ti_rpkg_drop(rpkg);
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
    if (ti_room_has_listeners(room) && !ti_is_shutting_down())
        room__emit_delete(room);
    else
        vec_destroy(room->listeners, (vec_destroy_cb) ti_watch_drop);

    free(room);
}

int ti_room_gen_id(ti_room_t * room)
{
    assert (!room->id);

    room->id = ti_next_free_id();
    return ti_room_to_map(room);
}

ti_watch_t * ti_room_join(ti_room_t * room, ti_stream_t * stream)
{
    ti_watch_t * watch;

    for (vec_each(room->listeners, ti_watch_t, watch))
    {
        if (watch->stream == stream)
            return watch;

        if (!watch->stream)
        {
            watch->stream = stream;
            goto finish;
        }
    }

    watch = ti_watch_create(stream);
    if (!watch)
        return NULL;

    if (vec_push(&room->listeners, watch))
        goto failed;

finish:
    if (vec_push_create(&stream->watching, watch))
        goto failed;
    return watch;

failed:
    /* when this fails, a few bytes might leak */
    watch->stream = NULL;
    return NULL;
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
