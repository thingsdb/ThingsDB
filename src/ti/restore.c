/*
 * ti/restore.c
 */
#include <ti.h>
#include <tiinc.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/epkg.h>
#include <ti/epkg.inline.h>
#include <ti/event.h>
#include <ti/modules.h>
#include <ti/restore.h>
#include <util/buf.h>
#include <util/vec.h>
#include <uv.h>

static uv_timer_t restore__timer;
static _Bool restore__is_busy = false;
static ti_user_t * restore__user;

char * ti_restore_job(const char * fn, size_t n)
{
    char * data;
    buf_t buf;

    buf_init(&buf);

    buf_append_str(&buf, "tar ");
    buf_append_str(&buf, "--no-same-permissions ");
    buf_append_str(&buf, "--exclude=.lock ");
    buf_append_str(&buf, "--exclude=global_status");
    buf_append_str(&buf, "--exclude=node_backups.mp ");
    buf_append_str(&buf, "--exclude=.node ");
    buf_append_str(&buf, "--exclude=ti.mp ");
    buf_append_str(&buf, "-xzf \"");
    buf_append(&buf, fn, n);
    buf_append_str(&buf, "\" -C \"");
    buf_append_str(&buf, ti.cfg->storage_path);
    buf_append_str(&buf, "\" . 2>&1");

    if (buf_append(&buf, "\0", 1))
    {
        free(buf.data);
        return NULL;
    }

    data = buf.data;
    buf.data = NULL;

    return data;
}

int ti_restore_chk(const char * fn, size_t n, ex_t * e)
{
    char * job;
    char buffer[512];
    int rc;
    _Bool firstline = true;
    buf_t buf;
    FILE * fp;

    buf_init(&buf);

    buf_append_str(&buf, "tar ");
    buf_append_str(&buf, "-tf \"");
    buf_append(&buf, fn, n);
    buf_append_str(&buf, "\" ");
    buf_append_str(&buf, "./ti.mp ");
    buf_append_str(&buf, "./store/access_node.mp ");
    buf_append_str(&buf, "./store/access_thingsdb.mp ");
    buf_append_str(&buf, "./store/collections.mp ");
    buf_append_str(&buf, "./store/idstat.mp ");
    buf_append_str(&buf, "./store/names.mp ");
    buf_append_str(&buf, "./store/procedures.mp ");
    buf_append_str(&buf, "./store/users.mp ");
    buf_append_str(&buf, "2>&1");

    if (buf_write(&buf, '\0'))
    {
        free(buf.data);
        ex_set_mem(e);
        return e->nr;
    }

    job = buf.data;

    buf_init(&buf);

    fp = popen(job, "r");
    if (!fp)
    {
        ex_set(e, EX_OPERATION, "failed to start `restore` task");
    }
    else
    {
        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            size_t sz = strlen(buffer);
            if (firstline && sz > 5 && memcmp(buffer, "tar: ", 5) == 0)
            {
                /*
                 * This captures the first line starting with `tar: ` which
                 * usually contains the error if one occurs.
                 */
                firstline = false;
                buf_append(&buf, buffer, sz);
            }
        }

        rc = pclose(fp);
        if (rc)
        {
            if (buf.len)
                ex_set(e, EX_BAD_DATA, "%.*s", buf.len-1, buf.data);
            else
                ex_set(e, EX_BAD_DATA, "invalid tar file");
        }
    }

    free(job);
    free(buf.data);
    return e->nr;
}

int ti_restore_unp(const char * restore_job, ex_t * e)
{
    char buffer[512];
    int rc;
    FILE * fp;
    buf_t buf;
    buf_init(&buf);

    fp = popen(restore_job, "r");
    if (!fp)
    {
        ex_set(e, EX_OPERATION, "failed to start `restore` task");
    }
    else
    {
        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            size_t sz = strlen(buffer);
            if (sz)
                buf_append(&buf, buffer, sz);
        }

        rc = pclose(fp);
        if (rc)
            ex_set(e, EX_OPERATION, "restore failed: `%.*s`",
                    buf.len, buf.data);
    }

    free(buf.data);
    return e->nr;
}

static void restore__cb(void)
{
    queue_t * queue;

    uv_close((uv_handle_t *) &restore__timer, NULL);

    /* make sure the event queue is empty */
    queue = ti.events->queue;
    while (queue->n)
        ti_event_drop(queue_pop(queue));

    /* cleanup the archive queue */
    queue = ti.archive->queue;
    while (queue->n)
        ti_epkg_drop(queue_pop(queue));

    /* reset all node status properties */
    ti.node->cevid = 0;
    ti.node->sevid = 0;
    ti.node->next_thing_id = 0;
    ti.nodes->cevid = 0;
    ti.nodes->sevid = 0;

    /* make sure we forget nodes info */
    ti.args->forget_nodes = 1;

    /* write global status (write zero status) */
    (void) ti_nodes_write_global_status();
}

static void restore__user_access(void)
{
    if (!restore__user)
        return;

    msgpack_packer pk;
    msgpack_sbuffer buffer;
    uint64_t event_id = ti.events->next_event_id++;
    uint64_t scope_id = 0;                      /* TI_SCOPE_THINGSDB */
    uint64_t thing_id = 0;                      /* parent root thing */
    uint64_t user_id = 1;                       /* overwrites the first user */
    ti_epkg_t * epkg;
    ti_pkg_t * pkg;
    ti_event_t * event;
    size_t njobs = 3 +
            !!restore__user->encpass +
            restore__user->tokens->n;

    if (mp_sbuffer_alloc_init(&buffer, njobs * 128, sizeof(ti_pkg_t)))
    {
        log_critical(EX_MEMORY_S);
        return;
    }
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    msgpack_pack_map(&pk, 1);

    msgpack_pack_array(&pk, 2);
    msgpack_pack_uint64(&pk, event_id);
    msgpack_pack_uint64(&pk, scope_id);

    msgpack_pack_map(&pk, 1);

    msgpack_pack_uint64(&pk, thing_id);
    msgpack_pack_array(&pk, njobs);

    msgpack_pack_map(&pk, 1);           /* job 1 */

    mp_pack_str(&pk, "clear_users");
    msgpack_pack_true(&pk);

    msgpack_pack_map(&pk, 1);           /* job 2 */

    mp_pack_str(&pk, "new_user");
    msgpack_pack_map(&pk, 3);

    mp_pack_str(&pk, "id");
    msgpack_pack_uint64(&pk, user_id);

    mp_pack_str(&pk, "username");
    mp_pack_strn(&pk, restore__user->name->data, restore__user->name->n);

    mp_pack_str(&pk, "created_at");
    msgpack_pack_uint64(&pk, restore__user->created_at);

    msgpack_pack_map(&pk, 1);           /* job 3 */

    mp_pack_str(&pk, "take_access");
    msgpack_pack_uint64(&pk, user_id);

    /* restore password (if required) */
    if (restore__user->encpass)
    {
        msgpack_pack_map(&pk, 1);

        mp_pack_str(&pk, "set_password");
        msgpack_pack_map(&pk, 2);

        mp_pack_str(&pk, "id");
        msgpack_pack_uint64(&pk, user_id);

        mp_pack_str(&pk, "password");
        mp_pack_str(&pk, restore__user->encpass);
    }

    /* restore tokens (if required) */
    for (vec_each(restore__user->tokens, ti_token_t, token))
    {
        msgpack_pack_map(&pk, 1);

        mp_pack_str(&pk, "new_token");
        msgpack_pack_map(&pk, 5);

        mp_pack_str(&pk, "id");
        msgpack_pack_uint64(&pk, user_id);

        mp_pack_str(&pk, "key");
        mp_pack_strn(&pk, token->key, sizeof(ti_token_key_t));

        mp_pack_str(&pk, "expire_ts");
        msgpack_pack_uint64(&pk, token->expire_ts);

        mp_pack_str(&pk, "created_at");
        msgpack_pack_uint64(&pk, token->created_at);

        mp_pack_str(&pk, "description");
        mp_pack_str(&pk, token->description);
    }

    pkg = (ti_pkg_t *) buffer.data;
    pkg_init(pkg, 0, TI_PROTO_NODE_EVENT, buffer.size);

    epkg = ti_epkg_create(pkg, event_id);
    if (!epkg)
        goto fail0;

    event = ti_event_epkg(epkg);
    if (!event)
        goto fail1;

    if (queue_push(&ti.events->queue, event))
        goto fail2;

    ti_user_drop(restore__user);
    return;  /* success */

fail2:
    free(event);
fail1:
    free(epkg);
fail0:
    free(pkg);
    ti.events->next_event_id--;
    log_critical("failed to create (users) job");
}

static void restore__master_cb(uv_timer_t * UNUSED(timer))
{
    restore__cb();

    if (ti_store_restore() || ti_archive_load())
    {
        log_critical("restore ThingsDB has failed");
        return;
    }

    restore__user_access();

    /* write global status (write loaded status) */
    (void) ti_nodes_write_global_status();

    restore__is_busy = false;
}

static void restore__slave_cb(uv_timer_t * UNUSED(timer))
{
    if (ti.futures_count)
    {
        log_warning("cancel all open futures before starting the restore...");
        ti_modules_cancel_futures();
        return;
    }

    restore__cb();

    (void) ti_store_init();
    (void) ti_archive_init();

    if (ti_sync_create() == 0)
        ti_sync_start();

    restore__is_busy = false;
}

_Bool ti_restore_is_busy(void)
{
    return restore__is_busy;
}

int ti_restore_master(ti_user_t * user /* may be NULL */)
{
    restore__is_busy = true;
    restore__user = ti_grab(user);

    if (uv_timer_init(ti.loop, &restore__timer))
        return -1;

    return uv_timer_start(&restore__timer, restore__master_cb, 150, 0);
}

int ti_restore_slave(void)
{
    restore__is_busy = true;

    ti_set_and_broadcast_node_status(TI_NODE_STAT_SYNCHRONIZING);

    if (uv_timer_init(ti.loop, &restore__timer))
        return -1;

    return uv_timer_start(&restore__timer, restore__slave_cb, 150, 1000);
}
