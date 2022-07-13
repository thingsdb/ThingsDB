/*
 * ti/backup.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/backup.h>
#include <ti/raw.inline.h>
#include <ti/val.inline.h>
#include <time.h>
#include <util/buf.h>
#include <util/fx.h>
#include <util/util.h>

#define TEMPLACE_CMP(pt__, end__, s__, sz__) \
*(pt__) == *(s__) && (pt__ + sz__) <= end__ && memcmp(pt__, s__, sz__) == 0

ti_backup_t * ti_backup_create(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t next_run,
        uint64_t repeat,
        uint64_t max_files,
        uint64_t created_at,
        queue_t * files)
{
    ti_backup_t * backup = malloc(sizeof(ti_backup_t));
    if (!backup)
        return NULL;

    /* files queue should be able to hold at least `max_files` number
     * of files. */
    assert (files->sz >= max_files);

    backup->id = id;
    backup->fn_template = strndup(fn_template, fn_templare_n);
    backup->result_msg = NULL;
    backup->work_fn = NULL;
    backup->files = NULL;
    backup->next_run = next_run;
    backup->repeat = repeat;
    backup->max_files = max_files;
    backup->scheduled = true;
    backup->result_code = 0;
    backup->created_at = created_at;
    if (!backup->fn_template)
    {
        ti_backup_destroy(backup);
        return NULL;
    }

    backup->files = files;  /* set only when everything is successful */
    return backup;
}

void ti_backup_destroy(ti_backup_t * backup)
{
    if (!backup)
        return;
    free(backup->fn_template);
    free(backup->result_msg);
    ti_val_drop((ti_val_t *) backup->work_fn);
    queue_destroy(backup->files, (queue_destroy_cb) ti_val_unsafe_drop);
    free(backup);
}

char * backup__next_run(ti_backup_t * backup)
{
    /* length 27 =  "2000-00-00 00:00:00+01.00Z" + 1 */
    static char buf[27];
    struct tm * tm_info;
    uint64_t now = util_now_usec();

    if (backup->next_run < now)
        return "pending";

    tm_info = gmtime((const time_t *) &backup->next_run);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%SZ", tm_info);
    return buf;
}

int ti_backup_info_to_pk(ti_backup_t * backup, msgpack_packer * pk)
{
    size_t n = 4;
    if (backup->scheduled)
        n += 1;
    if (backup->repeat)
        n += 2;
    if (backup->result_msg)
        n += 2;
    if (
        msgpack_pack_map(pk, n) ||

        mp_pack_str(pk, "id") ||
        msgpack_pack_uint64(pk, backup->id) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, backup->created_at) ||

        mp_pack_str(pk, "file_template") ||
        mp_pack_str(pk, backup->fn_template)
    ) return -1;

    if (backup->scheduled && (
        mp_pack_str(pk, "next_run") ||
        mp_pack_str(pk, backup__next_run(backup))
    )) return -1;

    if (backup->repeat && (
        mp_pack_str(pk, "repeat") ||
        msgpack_pack_uint64(pk, backup->repeat) ||
        mp_pack_str(pk, "max_files") ||
        msgpack_pack_uint64(pk, backup->max_files)
    )) return -1;

    if (backup->result_msg && (
        mp_pack_str(pk, "result_message") ||
        mp_pack_str(pk, backup->result_msg) ||

        mp_pack_str(pk, "result_code") ||
        msgpack_pack_int(pk, backup->result_code)
    )) return -1;

    if (mp_pack_str(pk, "files") ||
        msgpack_pack_array(pk, backup->files->n)
    ) return -1;

    for (queue_each(backup->files, ti_raw_t, fn))
        if (mp_pack_strn(pk, fn->data, fn->n))
            return -1;

    return 0;
}

ti_val_t * ti_backup_as_mpval(ti_backup_t * backup)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_backup_info_to_pk(backup, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}

_Bool ti_backup_is_gcloud(ti_backup_t * backup)
{
    char * s = backup->fn_template;

    return  s[0] == 'g' &&
            s[1] == 's' &&
            s[2] == ':' &&
            s[3] == '/' &&
            s[4] == '/';
}

char * ti_backup_gcloud_task(ti_backup_t * backup)
{
    struct tm * tm_info;
    uint64_t now = util_now_usec();
    const size_t change_id_sz = strlen("{CHANGE_ID}");
    const size_t date_sz = strlen("{DATE}");
    const size_t time_sz = strlen("{TIME}");
    char buffer[512];
    buf_t buf;
    buf_t fnbuf;
    buf_t gsbuf;
    char * pv = backup->fn_template;
    char * pt = backup->fn_template;
    char * end = pt + strlen(backup->fn_template);

    buf_init(&buf);
    buf_init(&fnbuf);
    buf_init(&gsbuf);

    if (buf_append_fmt(
            &fnbuf,
            "/tmp/_tmpbackup_%"PRIu32"_%"PRIu64".tar.gz",
            ti.node->id,
            backup->id))
        return NULL;

    tm_info = gmtime((const time_t *) &now);

    buf_append_str(
            &buf,
            "tar "
            "--exclude=.lock "
            "--exclude=*.tar.gz "
            "--exclude=*backup* "
            "--exclude=lost+found "
            "-czf \"");

    buf_append(&buf, fnbuf.data, fnbuf.len);

    while(*pt)
    {
        if (TEMPLACE_CMP(pt, end, "{CHANGE_ID}", change_id_sz))
        {
            buf_append(&gsbuf, pv, pt - pv);
            sprintf(buffer, "%"PRIu64, ti.node->ccid);
            buf_append_str(&gsbuf, buffer);
            pt += change_id_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{DATE}", date_sz))
        {
            buf_append(&gsbuf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%Y%m%d", tm_info);
            buf_append_str(&gsbuf, buffer);
            pt += date_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{TIME}", time_sz))
        {
            buf_append(&gsbuf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%H%M%S", tm_info);
            buf_append_str(&gsbuf, buffer);
            pt += time_sz;
            pv = pt;
            continue;
        }

        ++pt;
    }

    buf_append(&gsbuf, pv, pt - pv);

    ti_val_drop((ti_val_t *) backup->work_fn);
    backup->work_fn = ti_str_create(gsbuf.data, gsbuf.len);

    buf_append_fmt(&buf, "\" -C \"%s\" . 2>&1; ", ti.cfg->storage_path);

    if (ti.cfg->gcloud_key_file)
        buf_append_fmt(
                &buf,
                "gcloud auth activate-service-account "
                "--quiet --key-file \"%s\" 2>&1; ",
                ti.cfg->gcloud_key_file);

    buf_append_fmt(
            &buf,
            "gsutil -o 'Boto:num_retries=1' cp %.*s %.*s 2>&1; "
            "RETURN=$? 2>&1; "
            "rm %.*s 2>&1; "
            "exit $RETURN;",
            fnbuf.len, fnbuf.data,
            gsbuf.len, gsbuf.data,
            fnbuf.len, fnbuf.data);

    if (!backup->work_fn || buf_write(&buf, '\0'))
    {
        ti_val_drop((ti_val_t *) backup->work_fn);
        free(buf.data);
        backup->work_fn = NULL;
        buf.data = NULL;
    }

    free(fnbuf.data);
    free(gsbuf.data);

    return buf.data;
}

char * ti_backup_task(ti_backup_t * backup)
{
    struct tm * tm_info;
    uint64_t now = util_now_usec();
    const size_t change_id_sz = strlen("{CHANGE_ID}");
    const size_t date_sz = strlen("{DATE}");
    const size_t time_sz = strlen("{TIME}");
    char buffer[512];
    buf_t buf;
    char * pv = backup->fn_template;
    char * pt = backup->fn_template;
    char * end = pt + strlen(backup->fn_template);
    size_t offset, path_n = 0;

    buf_init(&buf);
    tm_info = gmtime((const time_t *) &now);

    buf_append_str(
            &buf,
            "tar "
            "--exclude=.lock "
            "--exclude=*.tar.gz "
            "--exclude=*backup* "
            "--exclude=lost+found "
            "-czf \"");

    offset = buf.len;

    while(*pt)
    {
        if (TEMPLACE_CMP(pt, end, "{CHANGE_ID}", change_id_sz))
        {
            buf_append(&buf, pv, pt - pv);
            sprintf(buffer, "%"PRIu64, ti.node->ccid);
            buf_append_str(&buf, buffer);
            pt += change_id_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{DATE}", date_sz))
        {
            buf_append(&buf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%Y%m%d", tm_info);
            buf_append_str(&buf, buffer);
            pt += date_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{TIME}", time_sz))
        {
            buf_append(&buf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%H%M%S", tm_info);
            buf_append_str(&buf, buffer);
            pt += time_sz;
            pv = pt;
            continue;
        }

        if (*pt == '/')
            path_n = buf.len + (pt - pv);

        ++pt;
    }

    buf_append(&buf, pv, pt - pv);

    ti_val_drop((ti_val_t *) backup->work_fn);
    backup->work_fn = ti_str_create(buf.data + offset, buf.len - offset);

    if (path_n > 1)
        (void) fx_mkdir_n(buf.data + offset, path_n - offset);

    buf_append_str(&buf, "\" -C \"");
    buf_append_str(&buf, ti.cfg->storage_path);
    buf_append_str(&buf, "\" . 2>&1;");

    if (!backup->work_fn || buf_write(&buf, '\0'))
    {
        free(buf.data);
        buf.data = NULL;
    }

    return buf.data;
}
