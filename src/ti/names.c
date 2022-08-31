/*
 * names.h
 */
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/names.h>
#include <ti/val.h>
#include <util/logger.h>
#include <util/vec.h>

smap_t * names;


int ti_names_create(void)
{
    names = ti.names = smap_create();
    return -(names == NULL);
}

int names__no_ref_cb(ti_name_t * name, void * UNUSED(arg))
{
    /* actually, only common variable should exist in here; */
    return name->ref == 1 ? 0 : -1;
}

_Bool ti_names_no_ref(void)
{
    const uint32_t COMMON_NAMES = 24;
    if (names->n != COMMON_NAMES)
        return -1;
    return smap_values(names, (smap_val_cb) names__no_ref_cb, NULL) == 0;
}

void ti_names_destroy(void)
{
    if (!names)
        return;
    smap_destroy(names, free);
    names = ti.names = NULL;
}

/*
 * non-critical function, just to inject some common names
 */
void ti_names_inject_common(void)
{
    (void) ti_names_from_str("_");
    (void) ti_names_from_str("a");
    (void) ti_names_from_str("b");
    (void) ti_names_from_str("c");
    (void) ti_names_from_str("x");
    (void) ti_names_from_str("y");
    (void) ti_names_from_str("z");
    (void) ti_names_from_str("tmp");
#ifdef TI_PRELOAD_NAMES
    (void) ti_names_from_str("access");
    (void) ti_names_from_str("archive_files");
    (void) ti_names_from_str("archived_in_memory");
    (void) ti_names_from_str("arguments");
    (void) ti_names_from_str("cache_expiration_time");
    (void) ti_names_from_str("cached_names");
    (void) ti_names_from_str("cached_queries");
    (void) ti_names_from_str("client_port");
    (void) ti_names_from_str("collection_id");
    (void) ti_names_from_str("conf");
    (void) ti_names_from_str("connected_clients");
    (void) ti_names_from_str("created_at");
    (void) ti_names_from_str("created_on");
    (void) ti_names_from_str("db_stored_change_id");
    (void) ti_names_from_str("default");
    (void) ti_names_from_str("definition");
    (void) ti_names_from_str("doc");
    (void) ti_names_from_str("enum_id");
    (void) ti_names_from_str("events_in_queue");
    (void) ti_names_from_str("expiration_time");
    (void) ti_names_from_str("fields");
    (void) ti_names_from_str("file_template");
    (void) ti_names_from_str("file");
    (void) ti_names_from_str("files");
    (void) ti_names_from_str("global_committed_change_id");
    (void) ti_names_from_str("global_stored_change_id");
    (void) ti_names_from_str("has_password");
    (void) ti_names_from_str("http_api_port");
    (void) ti_names_from_str("http_status_port");
    (void) ti_names_from_str("id");
    (void) ti_names_from_str("ip_support");
    (void) ti_names_from_str("key");
    (void) ti_names_from_str("libcleri_version");
    (void) ti_names_from_str("libpcre2_version");
    (void) ti_names_from_str("libuv_version");
    (void) ti_names_from_str("local_committed_change_id");
    (void) ti_names_from_str("local_stored_change_id");
    (void) ti_names_from_str("log_level");
    (void) ti_names_from_str("max_files");
    (void) ti_names_from_str("members");
    (void) ti_names_from_str("methods");
    (void) ti_names_from_str("modified_at");
    (void) ti_names_from_str("msgpack_version");
    (void) ti_names_from_str("name");
    (void) ti_names_from_str("next_free_id");
    (void) ti_names_from_str("next_run");
    (void) ti_names_from_str("next_thing_id");
    (void) ti_names_from_str("node_id");
    (void) ti_names_from_str("node_name");
    (void) ti_names_from_str("node_port");
    (void) ti_names_from_str("privileges");
    (void) ti_names_from_str("relations");
    (void) ti_names_from_str("repeat");
    (void) ti_names_from_str("restarts");
    (void) ti_names_from_str("result_code");
    (void) ti_names_from_str("result_message");
    (void) ti_names_from_str("result_size_limit");
    (void) ti_names_from_str("scheduled_backups");
    (void) ti_names_from_str("scope");
    (void) ti_names_from_str("status");
    (void) ti_names_from_str("storage_path");
    (void) ti_names_from_str("syntax_version");
    (void) ti_names_from_str("tasks");
    (void) ti_names_from_str("threshold_query_cache");
    (void) ti_names_from_str("tokens");
    (void) ti_names_from_str("type_id");
    (void) ti_names_from_str("uptime");
    (void) ti_names_from_str("user_id");
    (void) ti_names_from_str("user");
    (void) ti_names_from_str("version");
    (void) ti_names_from_str("with_side_effects");
    (void) ti_names_from_str("wrap_only");
    (void) ti_names_from_str("yajl_version");
    (void) ti_names_from_str("zone");
#endif
}

/*
 * returns a name with a new reference or NULL in case of an error
 */
ti_name_t * ti_names_new(const char * str, size_t n)
{
    ti_name_t * name = ti_name_create(str, n);
    if (!name || smap_add(names, name->str, name))
    {
        free(name);
        return NULL;
    }
    return name;
}

