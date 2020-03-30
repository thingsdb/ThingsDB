/*
 * ti/restore.c
 */
#include <ti/restore.h>
#include <util/buf.h>
#include <util/vec.h>
#include <uv.h>
#include <ti.h>

static uv_timer_t restore__timer;

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
    buf_append_str(&buf, "\" ./ti.mp 2>&1");

    if (buf_append(&buf, "\0", 1))
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
        ex_set(e, EX_OPERATION_ERROR, "failed to start `restore` task");
    }
    else
    {
        while (fgets(buffer, sizeof(buffer), fp) != NULL)
        {
            size_t sz = strlen(buffer);
            if (firstline && sz)
            {
                firstline = false;
                buf_append(&buf, buffer, sz);
            }
        }

        rc = pclose(fp);
        if (rc)
        {
            if (buf.len)
                ex_set(e, EX_BAD_DATA, "%.*s",
                    (int) buf.len-1, buf.data);
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
        ex_set(e, EX_OPERATION_ERROR, "failed to start `restore` task");
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
            ex_set(e, EX_OPERATION_ERROR, "restore failed: `%.*s`",
                    (int) buf.len, buf.data);
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

    /* write global status (write zero status) */
    (void) ti_nodes_write_global_status();
}

static void restore__master_cb(uv_timer_t * UNUSED(timer))
{
    restore__cb();

    if (ti_store_restore() || ti_archive_load())
    {
        log_critical("restore Things has failed");
        return;
    }

    /* write global status (write loaded status) */
    (void) ti_nodes_write_global_status();
}

static void restore__slave_cb(uv_timer_t * UNUSED(timer))
{
    restore__cb();

    (void) ti_store_init();
    (void) ti_archive_init();

    ti_set_and_broadcast_node_status(TI_NODE_STAT_SYNCHRONIZING);

    if (ti_sync_create() == 0)
        ti_sync_start();
}

int ti_restore_master(void)
{
    if (uv_timer_init(ti.loop, &restore__timer))
        return -1;

    return uv_timer_start(&restore__timer, restore__master_cb, 150, 0);
}

int ti_restore_slave(void)
{
    if (uv_timer_init(ti.loop, &restore__timer))
        return -1;

    return uv_timer_start(&restore__timer, restore__slave_cb, 150, 0);
}
