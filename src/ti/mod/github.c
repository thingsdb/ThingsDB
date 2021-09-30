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
#include <ti/mod/manifest.h>
#include <ti/mod/work.t.h>
#include <ti/module.h>
#include <tiinc.h>
#include <util/buf.h>
#include <util/logger.h>

#define GH__EXAMPLE "github.com/owner/repo[:token][@tag/branch]"
#define GH__MAX_LEN 4096
#define GH__API "https://api.github.com/repos/%s/%s/contents/%s"
#define GH__API_REF GH__API"?ref=%s"

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

static char * gh__url(ti_mod_github_t * gh, const char * fn)
{
    char * url;
    if (gh->ref)
        (void) asprintf(&url, GH__API_REF,gh->owner, gh->repo, fn, gh->ref);
    else
        (void) asprintf(&url, GH__API, gh->owner, gh->repo, fn);
    return url;
}

ti_mod_github_t * ti_mod_github_create(const char * s, size_t n, ex_t * e)
{
    int ok = 1;
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

    if (token)
        ok = asprintf(
                &gh->token_header,
                "Authorization: token %.*s",
                (int) tokenn, token) > 0;

    if (ok && ref)
        ok = (gh->ref = strndup(ref, refn)) != NULL;

    if (!ok || !gh->owner || !gh->repo ||
        !(gh->manifest_url = gh__url(gh, TI_MANIFEST)))
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

typedef size_t (*gh__write_cb) (void *, size_t, size_t, void *);

static size_t gh__write_mem(
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

static size_t gh__write_file(
        void * contents,
        size_t size,
        size_t nmemb,
        void * userp)
{
    return fwrite(contents, size, nmemb, userp);
}

CURLcode gh__download_url(
        const char * url,
        const char * token,
        void * data,
        gh__write_cb write_cb)
{
    CURLcode curle_code = CURLE_FAILED_INIT;
    CURL * curl = curl_easy_init();
    struct curl_slist * chunk = NULL;
    long http_code;

    if (token)
        chunk = curl_slist_append(chunk, token);

    chunk = curl_slist_append(chunk, "Accept: application/vnd.github.v3.raw");
    if(!curl || !chunk)
        goto cleanup;  /* CURLE_FAILED_INIT */

    LOGC(url);
    /* set our custom set of headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curle_code = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code < 200 || http_code > 299)
    {
        log_error("failed downloading %s (%ld)", url, http_code);
        curle_code = CURLE_HTTP_RETURNED_ERROR;
    }

cleanup:
    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
    return curle_code;
}

CURLcode gh__download_file(ti_module_t * module, const char * fn, mode_t mode)
{
    ti_mod_github_t * gh = module->source.github;
    CURLcode curle_code;
    FILE * fp;
    size_t n,
           path_n = strlen(module->path);
    int ok;
    const char * pt;
    char * dest,
         * url = gh__url(gh, fn);
    if (!url)
        return CURLE_FAILED_INIT;

    if (!fx_is_dir(module->path) && mkdir(module->path, FX_DEFAULT_DIR_ACCESS))
    {
        log_warn_errno_file("cannot create directory", errno, module->path);
        curle_code = CURLE_WRITE_ERROR;
        goto cleanup;
    }

    for (pt = fn, n = 0; *pt; ++n, ++pt)
    {
        if (*pt == '/')
        {
            dest = fx_path_join_strn(
                    module->path, path_n,
                    fn, n);
            if (!dest)
            {
                curle_code = CURLE_FAILED_INIT;
                goto cleanup;
            }

            ok = fx_is_dir(dest) || mkdir(dest, FX_DEFAULT_DIR_ACCESS) == 0;
            free(dest);
            if (!ok)
            {
                log_warn_errno_file("cannot create directory", errno, dest);
                curle_code = CURLE_WRITE_ERROR;
                goto cleanup;
            }
        }
    }

    dest = fx_path_join_strn(module->path, path_n, fn, n);
    if (!dest)
    {
        curle_code = CURLE_FAILED_INIT;
        goto cleanup;
    }

    fp = fopen(dest, "w");
    if (!fp)
    {
        log_errno_file("cannot open file", errno, dest);
        curle_code = CURLE_WRITE_ERROR;
    }
    else
    {
        curle_code = gh__download_url(
                url,
                gh->token_header,
                fp,
                gh__write_file);

        if ((fclose(fp) || chmod(dest, mode)) && curle_code == CURLE_OK)
            curle_code = CURLE_WRITE_ERROR;
    }

    free(dest);

cleanup:
    free(url);
    return curle_code;
}

int gh__download(ti_mod_work_t * data)
{
    ti_module_t * module = data->module;
    ti_mod_manifest_t * manifest = &data->manifest;
    CURLcode curle_code;

    /* read, write, execute, only by owner */
    curle_code = gh__download_file(module, manifest->main, S_IRWXU);
    if (curle_code != CURLE_OK)
    {
        ti_module_set_source_err(
                module, "failed to download `%s` (%s)",
                manifest->main,
                curl_easy_strerror(curle_code));
        return -1;
    }

    return 0;
}

void ti_mod_github_install(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;
    ti_mod_github_t * gh = module->source.github;
    buf_t buf;
    CURLcode curle_code;

    buf_init(&buf);

    curle_code = gh__download_url(
            gh->manifest_url,
            gh->token_header,
            &buf,
            gh__write_mem);
    if (curle_code != CURLE_OK)
    {
        ti_module_set_source_err(
                module, "failed to download "TI_MANIFEST" (%s)",
                curl_easy_strerror(curle_code));
        goto cleanup;  /* error */
    }

    if (ti_mod_manifest_read(
            &data->manifest,
            module->source_err,
            buf.data,
            buf.len))
        goto cleanup;  /* error */

    if (ti_mod_manifest_skip_install(&data->manifest, module))
        goto cleanup;  /* success, correct module is already installed */

    if (gh__download(data))
        goto cleanup;  /* error */

    /* we do not even care if storing the module file succeeds */
    (void) ti_mod_manifest_store(module->path, buf.data, buf.len);

cleanup:
    free(buf.data);
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
