/*
 * ti/backup.h
 */
#include <ti/backup.h>
#include <stdlib.h>
#include <string.h>
#include <util/util.h>
#include <util/buf.h>
#include <ti.h>
#include <time.h>

#define TEMPLACE_CMP(pt__, end__, s__, sz__) \
*(pt__) == *(s__) && (pt__ + sz__) <= end__ && memcmp(pt__, s__, sz__) == 0

ti_backup_t * ti_backup_create(
        uint64_t id,
        const char * fn_template,
        size_t fn_templare_n,
        uint64_t timestamp,
        uint64_t repeat)
{
    ti_backup_t * backup = malloc(sizeof(ti_backup_t));
    if (!backup)
        return NULL;

    backup->id = id;
    backup->fn_template = strndup(fn_template, fn_templare_n);
    backup->result_msg = NULL;
    backup->timestamp = timestamp;
    backup->repeat = repeat;
    backup->scheduled = true;
    backup->result_code = 0;

    if (!backup->fn_template)
    {
        ti_backup_destroy(backup);
        return NULL;
    }

    return backup;
}

void ti_backup_destroy(ti_backup_t * backup)
{
    if (!backup)
        return;
    free(backup->fn_template);
    free(backup->result_msg);
    free(backup);
}

char * backup__next_run(ti_backup_t * backup)
{
    /* length 27 =  "2000-00-00 00:00:00+01.00Z" + 1 */
    static char buf[27];
    struct tm * tm_info;
    uint64_t now = util_now_tsec();

    if (backup->timestamp < now)
        return "pending";

    tm_info = gmtime((const time_t *) &now);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%SZ", tm_info);
    return buf;
}

int ti_backup_info_to_pk(ti_backup_t * backup, msgpack_packer * pk)
{
    size_t n = 2;
    if (backup->scheduled)
        n += 1;
    if (backup->repeat)
        n += 1;
    if (backup->result_msg)
        n += 2;
    if (
        msgpack_pack_map(pk, n) ||

        mp_pack_str(pk, "id") ||
        msgpack_pack_uint64(pk, backup->id) ||

        mp_pack_str(pk, "file_template") ||
        mp_pack_str(pk, backup->fn_template)
    ) return -1;

    if (backup->scheduled && (
        mp_pack_str(pk, "next_run") ||
        mp_pack_str(pk, backup__next_run(backup))
    )) return -1;

    if (backup->repeat && (
        mp_pack_str(pk, "repeat") ||
        msgpack_pack_uint64(pk, backup->repeat)
    )) return -1;

    if (backup->result_msg && (
        mp_pack_str(pk, "last_result_message") ||
        mp_pack_str(pk, backup->result_msg) ||

        mp_pack_str(pk, "last_result_code") ||
        msgpack_pack_int(pk, backup->result_code)
    )) return -1;

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
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}

char * ti_backup_job(ti_backup_t * backup)
{
    char * data;
    struct tm * tm_info;
    uint64_t now = util_now_tsec();
    const size_t event_sz = strlen("{EVENT}");
    const size_t date_sz = strlen("{DATE}");
    const size_t time_sz = strlen("{TIME}");
    char buffer[512];
    buf_t buf;
    char * pv = backup->fn_template;
    char * pt = backup->fn_template;
    char * end = pt + strlen(backup->fn_template);

    buf_init(&buf);
    tm_info = gmtime((const time_t *) &now);

    buf_append_str(&buf, "tar --exclude=.lock -czf \"");

    while(*pt)
    {
        if (TEMPLACE_CMP(pt, end, "{EVENT}", event_sz))
        {
            buf_append(&buf, pv, pt - pv);
            sprintf(buffer, "%"PRIu64, ti()->node->cevid);
            buf_append_str(&buf, buffer);
            pt += event_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{DATE}", date_sz))
        {
            buf_append(&buf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%Y_%m_%d", tm_info);
            buf_append_str(&buf, buffer);
            pt += date_sz;
            pv = pt;
            continue;
        }
        if (TEMPLACE_CMP(pt, end, "{TIME}", time_sz))
        {
            buf_append(&buf, pv, pt - pv);
            strftime(buffer, sizeof(buffer), "%H_%M_%S", tm_info);
            buf_append_str(&buf, buffer);
            pt += time_sz;
            pv = pt;
            continue;
        }

        ++pt;
    }

    buf_append(&buf, pv, pt - pv);

    buf_append_str(&buf, "\" -C \"");
    buf_append_str(&buf, ti()->cfg->storage_path);
    buf_append_str(&buf, "\" . 2>&1");
    buf_append(&buf, "\0", 1);

    data = buf.data;
    buf.data = NULL;
    return data;
}
