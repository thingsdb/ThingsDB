/*
 * ti/commit.c
 */
#include <ti/commit.h>
#include <ti/raw.inline.h>
#include <util/logger.h>
#include <util/util.h>

ti_commit_t * ti_commit_create(
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
    ti_val_unsafe_drop(commit->by);
fail2:
    ti_val_unsafe_drop(commit->message);
fail1:
    ti_val_unsafe_drop(commit->code);
fail0:
    free(commit);
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
    ti_val_unsafe_drop(commit->code);
    ti_val_unsafe_drop(commit->message);
    ti_val_unsafe_drop(commit->by);
    ti_val_drop(commit->err_msg);
    free(commit);
}

int ti_commit_set_history(vec_t ** commits, _Bool state)
{
    if (state)
    {
        if (*commits)
            return 0;

        *commits = vec_new(8);
        return !!(*commits);
    }
    vec_destroy(*commits, ti_commit_destroy);
    *commits = NULL;
    return 0;
}