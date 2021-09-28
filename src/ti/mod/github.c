/*
 * ti/mod/github.c
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti.h>
#include <ti/mod/github.h>
#include <ti/module.h>
#include <tiinc.h>
#include <util/buf.h>
#include <util/logger.h>

#define GH__EXAMPLE "github.com/owner/repo[:TOKEN][@TAG/BRANCH/COMMIT]"

static const char * gh__str = "github.com";
static const size_t gh__strn = 10;  /*  "github.com"         */
static const size_t gh__minn = 14;  /*  "github.com/o/r"     */

_Bool ti_mod_github_test(const char * s, size_t n)
{
    if (n < gh__strn)
        return false;
    return memcmp(s, gh__str, gh__strn) == 0;
}

static inline int gh__ischar(int c)
{
    return isalnum(c) || c == '-' || c == '_' || c == '.';
}

ti_mod_github_t * ti_mod_github_create(const char * s, size_t n, ex_t * e)
{
    const char * owner,
         * repo,
         * token = NULL,
         * ref = NULL;
    size_t ownern = 0,
           repon = 0,
           tokenn = 0,
           refn = 0,
           pos = gh__strn;
    ti_mod_github_t * gh = calloc(1, sizeof(ti_mod_github_t));

    if (!gh)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (!ti_mod_github_test(s, n) || n < gh__minn)
        goto invalid;

    if (s[pos++] != '/')
        goto invalid;

    owner = s + pos;

    do
    {
        if (s[pos] == '/')
            break;

        if (!gh__ischar(s[pos]))
            goto invalid;

        ownern++;
    }
    while (++pos < n);

    if (!ownern || ++pos >= n)
        goto invalid;

    repo = s + pos;

    do
    {
        if (s[pos] == ':')
        {
            if (++pos == n)
                goto invalid;

            token = s + pos;
            do
            {
                if (s[pos] == '@')
                    break;

                if (!gh__ischar(s[pos]))
                    goto invalid;

                tokenn++;
            }
            while (++pos < n);

            if (!tokenn)
                goto invalid;

            break;
        }

        if (s[pos] == '@')
            break;

        if (!gh__ischar(s[pos]))
            goto invalid;

        repon++;
    }
    while (++pos < n);

    if (!repon)
        goto invalid;

    if (pos < n)  /* s[pos] == '@' */
    {
        if (++pos == n)
            goto invalid;

        ref = s + pos;
        do
        {
            if (!isgraph(s[pos]))
                goto invalid;

            refn++;
        }
        while (++pos < n);
    }

    gh->owner = strndup(owner, ownern);
    gh->repo = strndup(repo, repon);
    gh->token = strndup(token, tokenn);
    gh->ref = strndup(ref, refn);

    if (!gh->owner || !gh->repo || !gh->token || !gh->ref)
    {
        ex_set_mem(e);
        goto fail;
    }

    return gh;
invalid:
    ex_set(e, EX_VALUE_ERROR,
            "not a valid GitHub module link; "
            "expecting something like "GH__EXAMPLE" but got `%.*s`",
            n, s);
fail:
    ti_mod_github_destroy(gh);
    return NULL;
}

static size_t gh__write_cb(
        void * contents,
        size_t size,
        size_t nmemb,
        void * userp)
{
    size_t realsize = size * nmemb;
    buf_t * buf = userp;
    /* return 0 when a write error has occurred */
    return buf_append(buf, contents, realsize) == 0 ? realsize : 0;
}

static void gh__download_json(uv_work_t * work)
{
    ti_module_t * module = work->data;
    ti_mod_github_t * gh = module->source.github;
    buf_t buf;
    struct curl_slist * chunk = NULL;
    CURL * curl = curl_easy_init();
    CURLcode curle_code;

    buf_init(&buf);

    if(!curl)
    {
        ti_mod_github_set_code(gh, GH_INTERNAL_ERROR);
        goto cleanup;
    }
    chunk = curl_slist_append(chunk, "Accept:application/vnd.github.v3.raw");

    /* set our custom set of headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/thingsdb/ThingsDB/README.md");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gh__write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curle_code = curl_easy_perform(curl);

    ti_mod_github_set_curl_code(gh, curle_code);

    if (curle_code != CURLE_OK)
    {
        (void) snprintf(
                module->source_err,
                TI_MODULE_MAX_ERR,
                "failed to download ")
        ti_mod_github_set_code(gh, GH_CURLE_MODULE_DOWNLOAD_ERROR);
        goto cleanup;
    }

cleanup:
    free(buf.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
}

static void gh__download_json_finish(uv_work_t * work, int status)
{
    ti_module_t * module = work->data;

    ti_mod_manifest_t * manifest = &module->source.github->download_manifest;

    if (module->source_err[0])
        module->status = TI_MODULE_STAT_SOURCE_ERR;

    if (status)
    {
        log_error(uv_strerror(status));
    }
    else if (module->status != TI_MODULE_STAT_INSTALLER_BUSY)
    {
        log_error("failed to install module: `%s` (%s)",
                module->name->str,
                ti_module_status_str(module));
    }
    else if (module->ref > 1)  /* do nothing if this is the last reference */
    {

        /* copy the manifest */
        module->manifest = *manifest;

        /* clear the temporary manifest */
        memset(manifest, 0, sizeof(ti_mod_manifest_t));

        if (ti_module_set_file(
                module,
                module->manifest.main,
                strlen(module->manifest.main)))
        {
            log_error(EX_MEMORY_S);
            module->status = TI_MODULE_STAT_NOT_INSTALLED;
        }
        else
        {
            module->status = TI_MODULE_STAT_NOT_LOADED;
            ti_module_load(module);
        }
    }

    ti_module_drop(module);
    free(work);
}

void ti_mod_github_install(ti_module_t * module)
{
    uv_work_t * work;
    int prev_status = module->status;

    module->status = TI_MODULE_STAT_INSTALLER_BUSY;
    module->source_err[0] = '\0';

    ti_incref(module);

    work = malloc(sizeof(uv_work_t));
    if (!work)
        goto fail0;

    work->data = module;

    if (uv_queue_work(
            ti.loop,
            work,
            gh__download_json,
            gh__download_json_finish))
        goto fail1;

    return;

fail1:
    free(work);
fail0:
    ti_decref(module);
    module->status = prev_status;
    log_error("failed install module: `%s`", module->name->str);
}

void ti_mod_github_destroy(ti_mod_github_t * gh)
{
    if (!gh)
        return;
    free(gh->owner);
    free(gh->repo);
    free(gh->token);
    free(gh->ref);
    free(gh);
}
