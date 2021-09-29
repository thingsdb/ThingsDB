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

#define GH__EXAMPLE "github.com/owner/repo[:token][@tag/branch]"
#define GH__MAX_LEN 4096


static const char * gh__str = "github.com";
static const size_t gh__strn = 10;  /*  "github.com"         */
static const size_t gh__minn = 14;  /*  "github.com/o/r"     */

_Bool ti_mod_github_test(const char * s, size_t n)
{
    if (n < gh__strn || n > GH__MAX_LEN)
        return false;
    return memcmp(s, gh__str, gh__strn) == 0;
}

static inline int gh__ischar(int c)
{
    return isalnum(c) || c == '-' || c == '_' || c == '.';
}

ti_mod_github_t * ti_mod_github_create(const char * s, size_t n, ex_t * e)
{
    int rc = 0;
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
    gh->ref = refn ? strndup(ref, refn) : strdup("default");

    if (token)
        rc = asprintf(
                &gh->token_header,
                "Authorization: token %.*s",
                (int) tokenn, token);

    rc = rc < 0 ? rc : refn
        ? asprintf(
                &gh->manifest_url,
                "https://api.github.com/%s/%s/"TI_MANIFEST"?ref=%s",
                gh->owner, gh->repo, gh->ref)
        : asprintf(
                &gh->manifest_url,
                "https://api.github.com/%s/%s/"TI_MANIFEST,
                gh->owner, gh->repo);

    if (rc < 0 || !gh->owner || !gh->repo || !gh->ref)
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

void ti_mod_github_get_manifest(uv_work_t * work)
{
    ti_module_t * module = work->data;
    ti_mod_github_t * gh = module->source.github;
    buf_t buf;
    struct curl_slist * chunk = NULL;
    CURL * curl = curl_easy_init();
    CURLcode curle_code;

    buf_init(&buf);

    if (gh->token_header)
        chunk = curl_slist_append(chunk, gh->token_header);

    chunk = curl_slist_append(chunk, "Accept: application/vnd.github.v3.raw");

    if(!curl || !chunk)
    {
        ti_module_set_source_err(
                module,
                "failed to download "TI_MANIFEST" (failed to initialize)");
        goto cleanup;
    }

    /* set our custom set of headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_URL, gh->manifest_url);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gh__write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curle_code = curl_easy_perform(curl);

    if (curle_code != CURLE_OK)
    {
        ti_module_set_source_err(
                module, "failed to download "TI_MANIFEST" (%s)",
                curl_easy_strerror(curle_code));
        goto cleanup;
    }

cleanup:
    free(buf.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
}


void ti_mod_github_destroy(ti_mod_github_t * gh)
{
    if (!gh)
        return;
    free(gh->owner);
    free(gh->repo);
    free(gh->ref);
    free(gh->token_header);
    free(gh->manifest_url);

    free(gh);
}
