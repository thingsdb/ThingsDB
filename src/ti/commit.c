/*
 * ti/commit.c
 */
#include <ti/commit.h>
#include <ti/datetime.h>
#include <ti/raw.inline.h>
#include <ti/val.inline.h>
#include <util/logger.h>
#include <util/util.h>

static ti_commit_t * commit_create(
        uint64_t id,
        time_t ts,
        const char * code,
        size_t code_n,
        const char * message,
        size_t message_n,
        const char * by,
        size_t by_n,
        const char * err_msg,
        size_t err_msg_n)
{
    ti_commit_t * commit = malloc(sizeof(ti_commit_t));
    if (!commit)
        return NULL;

    commit->id = id;
    commit->ts = ts;

    commit->code = ti_str_create(code, code_n);
    if (!commit->code)
        goto fail0;

    commit->message = ti_str_create(message, message_n);
    if (!commit->message)
        goto fail1;

    commit->by = ti_str_create(by, by_n);
    if (!commit->by)
        goto fail2;

    if (err_msg)
    {
        commit->err_msg = ti_str_create(err_msg, err_msg_n);
        if (!commit->err_msg)
            goto fail3;
    }
    else
        commit->err_msg = NULL;

    return commit;
fail3:
    ti_val_unsafe_drop((ti_val_t *) commit->by);
fail2:
    ti_val_unsafe_drop((ti_val_t *) commit->message);
fail1:
    ti_val_unsafe_drop((ti_val_t *) commit->code);
fail0:
    free(commit);
    return NULL;
}

ti_commit_t * ti_commit_from_up(mp_unp_t * up)
{
    mp_obj_t obj, mp_id, mp_ts, mp_code, mp_by, mp_message, mp_err_msg;
    if (mp_next(up, &obj) != MP_ARR || obj.via.sz != 6 ||
        mp_next(up, &mp_id) != MP_U64 ||
        mp_next(up, &mp_ts) != MP_U64 ||
        mp_next(up, &mp_code) != MP_STR ||
        mp_next(up, &mp_by) != MP_STR ||
        mp_next(up, &mp_message) != MP_STR ||
        (mp_next(up, &mp_err_msg) != MP_STR && mp_err_msg.tp == MP_NIL))
        return NULL;
    return commit_create(
        mp_id.via.u64,
        (time_t) mp_ts.via.u64,
        mp_code.via.str.data, mp_code.via.str.n,
        mp_message.via.str.data, mp_message.via.str.n,
        mp_by.via.str.data, mp_by.via.str.n,
        mp_err_msg.tp == MP_STR ? mp_err_msg.via.str.data : NULL,
        mp_err_msg.tp == MP_STR ? mp_err_msg.via.str.n : 0
    );
}

ti_commit_t * ti_commit_make(
        uint64_t id,
        const char * code,
        ti_raw_t * by,
        ti_raw_t * message)
{
    ti_commit_t * commit = malloc(sizeof(ti_commit_t));
    if (!commit)
        return NULL;

    commit->id = id;
    commit->ts = (time_t) util_now_usec();

    commit->code = ti_str_from_str(code);
    if (!commit->code)
    {
        free(commit);
        return NULL;
    }

    commit->by = ti_grab(by);
    commit->message = ti_grab(message);
    commit->err_msg = NULL;
    return commit;
}

void ti_commit_destroy(ti_commit_t * commit)
{
    if (!commit)
        return;
    ti_val_unsafe_drop((ti_val_t *) commit->code);
    ti_val_unsafe_drop((ti_val_t *) commit->message);
    ti_val_unsafe_drop((ti_val_t *) commit->by);
    ti_val_drop((ti_val_t *) commit->err_msg);
    free(commit);
}

int ti_commit_to_pk(ti_commit_t * commit, msgpack_packer * pk)
{
    return -(
        msgpack_pack_array(pk, 6) ||
        msgpack_pack_uint64(pk, commit->id) ||
        msgpack_pack_uint64(pk, (uint64_t) commit->ts) ||
        mp_pack_strn(pk, commit->code->data, commit->code->n) ||
        mp_pack_strn(pk, commit->message->data, commit->message->n) ||
        mp_pack_strn(pk, commit->by->data, commit->by->n) || (
            commit->err_msg
            ? mp_pack_strn(pk, commit->err_msg->data, commit->err_msg->n)
            : msgpack_pack_nil(pk)
        )
    );
}

int ti_commit_to_client_pk(
        ti_commit_t * commit,
        _Bool detail,
        msgpack_packer * pk)
{
    const char * isotimestr = ti_datetime_ts_str((const time_t *) &commit->ts);
    return -(
        msgpack_pack_map(pk, 4 + !!detail + !!commit->err_msg) ||

        mp_pack_str(pk, "id") ||
        msgpack_pack_uint64(pk, commit->id) ||

        mp_pack_str(pk, "created_on") ||
        mp_pack_str(pk, isotimestr) ||

        mp_pack_str(pk, "by") ||
        mp_pack_strn(pk, commit->by->data, commit->by->n) ||

        mp_pack_str(pk, "message") ||
        mp_pack_strn(pk, commit->message, commit->message->n) ||

        (detail && (
            mp_pack_str(pk, "code") ||
            mp_pack_strn(pk, commit->code, commit->code->n)
        )) ||

        (commit->err_msg && (
            mp_pack_str(pk, "err_msg") ||
            mp_pack_strn(pk, commit->err_msg, commit->err_msg->n)
        ))
    );
}

ti_val_t * ti_commit_as_mpval(ti_commit_t * commit, _Bool detail)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_commit_to_client_pk(commit, detail, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}
