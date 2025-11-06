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
    (void) ti_names_from_str_slow("_");
    (void) ti_names_from_str_slow("a");
    (void) ti_names_from_str_slow("b");
    (void) ti_names_from_str_slow("c");
    (void) ti_names_from_str_slow("x");
    (void) ti_names_from_str_slow("y");
    (void) ti_names_from_str_slow("z");
    (void) ti_names_from_str_slow("tmp");
#ifdef TI_PRELOAD_NAMES
    (void) ti_names_from_str_slow("access");
    (void) ti_names_from_str_slow("archive_files");
    (void) ti_names_from_str_slow("archived_in_memory");
    (void) ti_names_from_str_slow("arguments");
    (void) ti_names_from_str_slow("cache_expiration_time");
    (void) ti_names_from_str_slow("cached_names");
    (void) ti_names_from_str_slow("cached_queries");
    (void) ti_names_from_str_slow("client_port");
    (void) ti_names_from_str_slow("collection_id");
    (void) ti_names_from_str_slow("conf");
    (void) ti_names_from_str_slow("connected_clients");
    (void) ti_names_from_str_slow("created_at");
    (void) ti_names_from_str_slow("created_on");
    (void) ti_names_from_str_slow("db_stored_change_id");
    (void) ti_names_from_str_slow("default");
    (void) ti_names_from_str_slow("definition");
    (void) ti_names_from_str_slow("doc");
    (void) ti_names_from_str_slow("enum_id");
    (void) ti_names_from_str_slow("events_in_queue");
    (void) ti_names_from_str_slow("expiration_time");
    (void) ti_names_from_str_slow("fields");
    (void) ti_names_from_str_slow("file_template");
    (void) ti_names_from_str_slow("file");
    (void) ti_names_from_str_slow("files");
    (void) ti_names_from_str_slow("global_committed_change_id");
    (void) ti_names_from_str_slow("global_stored_change_id");
    (void) ti_names_from_str_slow("has_password");
    (void) ti_names_from_str_slow("http_api_port");
    (void) ti_names_from_str_slow("http_status_port");
    (void) ti_names_from_str_slow("id");
    (void) ti_names_from_str_slow("ip_support");
    (void) ti_names_from_str_slow("key");
    (void) ti_names_from_str_slow("libcleri_version");
    (void) ti_names_from_str_slow("libpcre2_version");
    (void) ti_names_from_str_slow("libuv_version");
    (void) ti_names_from_str_slow("local_committed_change_id");
    (void) ti_names_from_str_slow("local_stored_change_id");
    (void) ti_names_from_str_slow("log_level");
    (void) ti_names_from_str_slow("max_files");
    (void) ti_names_from_str_slow("members");
    (void) ti_names_from_str_slow("methods");
    (void) ti_names_from_str_slow("modified_at");
    (void) ti_names_from_str_slow("msgpack_version");
    (void) ti_names_from_str_slow("name");
    (void) ti_names_from_str_slow("next_free_id");
    (void) ti_names_from_str_slow("next_run");
    (void) ti_names_from_str_slow("next_thing_id");
    (void) ti_names_from_str_slow("node_id");
    (void) ti_names_from_str_slow("node_name");
    (void) ti_names_from_str_slow("node_port");
    (void) ti_names_from_str_slow("privileges");
    (void) ti_names_from_str_slow("relations");
    (void) ti_names_from_str_slow("repeat");
    (void) ti_names_from_str_slow("restarts");
    (void) ti_names_from_str_slow("result_code");
    (void) ti_names_from_str_slow("result_message");
    (void) ti_names_from_str_slow("result_size_limit");
    (void) ti_names_from_str_slow("scheduled_backups");
    (void) ti_names_from_str_slow("scope");
    (void) ti_names_from_str_slow("status");
    (void) ti_names_from_str_slow("storage_path");
    (void) ti_names_from_str_slow("syntax_version");
    (void) ti_names_from_str_slow("tasks");
    (void) ti_names_from_str_slow("threshold_query_cache");
    (void) ti_names_from_str_slow("tokens");
    (void) ti_names_from_str_slow("type_id");
    (void) ti_names_from_str_slow("uptime");
    (void) ti_names_from_str_slow("user_id");
    (void) ti_names_from_str_slow("user");
    (void) ti_names_from_str_slow("version");
    (void) ti_names_from_str_slow("with_side_effects");
    (void) ti_names_from_str_slow("wrap_only");
    (void) ti_names_from_str_slow("yajl_version");
    (void) ti_names_from_str_slow("zone");
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

ti_name_t * ti_names_get_slow(const char * str, size_t n)
{
    ti_name_t * name = smap_getn(names, str, n);
    if (name)
    {
        ti_incref(name);
        return name;
    }
    return ti_names_new(str, n);
}
