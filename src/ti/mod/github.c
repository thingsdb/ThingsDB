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
#include <yajl/yajl_parse.h>


#define GH__EXAMPLE "github.com/owner/repo[:token][@tag/branch]"
#define GH__MAX_LEN 4096
#define GH__API "https://api.github.com/repos/%s/%s/contents/%s"
#define GH__API_REF GH__API"?ref=%s"

#define GH__API_BLOB "https://api.github.com/repos/%s/%s/git/blobs/%.*s"
#define GH__API_BLOB_REF GH__API_BLOB"?ref=%s"

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

static inline int gh__isblob(const char * fn)
{
    return strncmp(fn, "bin/", 4) == 0 || strncmp(fn, "blob/", 5) == 0;
}

static char * gh__url(ti_mod_github_t * gh, const char * fn)
{
    char * url;
    if (gh->ref)
        (void) asprintf(&url, GH__API_REF, gh->owner, gh->repo, fn, gh->ref);
    else
        (void) asprintf(&url, GH__API, gh->owner, gh->repo, fn);
    return url;
}

static char * gh__blob_url(ti_mod_github_t * gh, const char * sha, int n)
{
    char * url;
    if (gh->ref)
        asprintf(&url, GH__API_BLOB_REF, gh->owner, gh->repo, n, sha, gh->ref);
    else
        asprintf(&url, GH__API_BLOB, gh->owner, gh->repo, n, sha);
    return url;
}

/*
 * START read JSON directory output for finding SHA url's
 */

enum
{
    /* set GH__FLAG_FOUND when the correct path is found at `level 1`. */
    GH__FLAG_FOUND      =1<<0,
    /*
     * Set GH__FLAG_DONE when closing a map and returning to `level 0` when
     * the flag `GH__FLAG_FOUND` has been set.
     * When the GH__FLAG_DONE is set, no longer switch from `sha`, otherwise
     * reset the `sha` to NULL when entering a map to `level 1`.
     */
    GH__FLAG_DONE       =1<<1,
};

enum
{
    GH__KEY_OTHER,
    GH__KEY_PATH,
    GH__KEY_SHA,
};

typedef struct
{
    int level;              /* search at level 1 */
    int flags;
    int key;
    const char * path;      /* this is what we are looking for */
    size_t path_n;
    const char * sha;       /* this is what want */
    size_t sha_n;
} gh__ctx_t;


static inline _Bool gh__equals(
        const void * a, size_t an,
        const void * b, size_t bn)
{
    return an == bn && memcmp(a, b, an) == 0;
}

static int gh__json_string(
        void * data,
        const unsigned char * s,
        size_t n)
{
    gh__ctx_t * ctx = data;
    switch (ctx->key)
    {
    case GH__KEY_OTHER:
        break;
    case GH__KEY_PATH:
        if (gh__equals(ctx->path, ctx->path_n, s, n))
            ctx->flags |= GH__FLAG_FOUND;
        break;
    case GH__KEY_SHA:
        ctx->sha = (const char *) s;
        ctx->sha_n = n;
        break;
    }
    return 1;
}

static int gh__json_start_map(void * data)
{
    gh__ctx_t * ctx = data;
    ++ctx->level;

    if (ctx->level == 1 && (~ctx->flags & GH__FLAG_DONE))
        ctx->sha = NULL;

    return 1;
}

static int gh__json_map_key(
        void * data,
        const unsigned char * s,
        size_t n)
{
    gh__ctx_t * ctx = data;

    if (ctx->level == 1)
    {
        if (gh__equals("path", 4, s, n))
        {
            ctx->key = GH__KEY_PATH;
            return 1;
        }

        if (gh__equals("sha", 3, s, n))
        {
            ctx->key = GH__KEY_SHA;
            return 1;
        }
    }

    ctx->key = GH__KEY_OTHER;

    return 1;
}

static int gh__json_end_map(void * data)
{
    gh__ctx_t * ctx = data;
    --ctx->level;

    if (ctx->level == 0 && (ctx->flags & GH__FLAG_FOUND))
        ctx->flags |= GH__FLAG_DONE;

    return 1;
}


static yajl_callbacks gh__callbacks = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gh__json_string,
    gh__json_start_map,
    gh__json_map_key,
    gh__json_end_map,
    NULL,
    NULL
};

static char * gh__blob_url_from_json(
        ti_mod_github_t * gh,
        const char * path,
        const void * data,
        size_t n)
{
    char * url;
    yajl_handle hand;
    gh__ctx_t ctx = {
            .flags = 0,
            .key = GH__KEY_OTHER,
            .level = 0,
            .path = path,
            .path_n = strlen(path),
            .sha = NULL,
            .sha_n = 0,
    };

    hand = yajl_alloc(&gh__callbacks, NULL, &ctx);
    if (!hand)
        return NULL;

    (void) yajl_parse(hand, data, n);

    url = (ctx.sha_n < INT_MAX && ctx.sha && (ctx.flags & GH__FLAG_DONE))
         ? gh__blob_url(gh, ctx.sha, (int) ctx.sha_n)
         : NULL;

    if (!url)
        log_error("failed to find SHA for `%s`", path);
    else
    {
        log_debug("found SHA `%s` for `%s`", url, path);
    }

    yajl_free(hand);
    return url;
}

int ti_mod_github_init(
        ti_mod_github_t * gh,
        const char * s,
        size_t n,
        ex_t * e)
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

    if (gh)
    {
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
            ex_set_mem(e);
    }

    return e->nr;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "not a valid GitHub module link; "
            "expecting something like "GH__EXAMPLE" but got `%.*s`",
            n, s);
    return e->nr;
}

ti_mod_github_t * ti_mod_github_create(const char * s, size_t n, ex_t * e)
{
    ti_mod_github_t * gh = calloc(1, sizeof(ti_mod_github_t));
    if (!gh)
    {
        ex_set_mem(e);
        return NULL;
    }

    if (ti_mod_github_init(gh, s, n, e))
    {
        ti_mod_github_destroy(gh);
        return NULL;
    }
    return gh;
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

static CURLcode gh__download_url(
        const char * url,
        const char * token,
        void * data,
        gh__write_cb write_cb,
        const char * option)  /* option = raw/json */
{
    CURLcode curle_code = CURLE_FAILED_INIT;
    CURL * curl = curl_easy_init();
    struct curl_slist * chunk = NULL;
    long http_code;

    if (token)
        chunk = curl_slist_append(chunk, token);

    chunk = curl_slist_append(chunk, *option == 'r'
            ? "Accept: application/vnd.github.v3.raw"
            : "Accept: application/vnd.github.v3+json");
    if(!curl || !chunk)
        goto cleanup;  /* CURLE_FAILED_INIT */

    /* set our custom set of headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    /* enable verbose logging when log level is debug */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, Logger.level == LOGGER_DEBUG);

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

static char * gh__get_blob_url(ti_mod_github_t * gh, const char * fn)
{
    assert (gh__isblob(fn));

    buf_t buf;
    CURLcode rc = CURLE_OK;
    char * url;

    buf_init(&buf);

    /* Only `bin/` and `blob/` folders are supported, see gh__isblob() */
    url = gh__url(gh, fn[1] == 'i' ? "bin" : "blob");
    if (!url)
        return NULL;

    rc = gh__download_url(url, gh->token_header, &buf, gh__write_mem, "json");

    free(url);

    if (rc != CURLE_OK)
    {
        log_error("failed to download directory json output (%s)",
                curl_easy_strerror(rc));
        url = NULL;
    }
    else
        url = gh__blob_url_from_json(gh, fn, buf.data, buf.len);

    free(buf.data);
    return url;
}

static CURLcode gh__download_file(
        ti_module_t * module,
        const char * fn,
        mode_t mode)
{
    ti_mod_github_t * gh = module->source.github;
    CURLcode curle_code;
    FILE * fp;
    size_t n,
           path_n = strlen(module->path);
    int ok;
    const char * pt;
    char * dest, * url = NULL;

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
        url = gh__isblob(fn) ? gh__get_blob_url(gh, fn) : gh__url(gh, fn);

        curle_code = url ? gh__download_url(
                url,
                gh->token_header,
                fp,
                gh__write_file,
                "raw") : CURLE_FAILED_INIT;

        if ((fclose(fp) || chmod(dest, mode)) && curle_code == CURLE_OK)
            curle_code = CURLE_WRITE_ERROR;
    }

    free(dest);

cleanup:
    free(url);
    return curle_code;
}

void ti_mod_github_download(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;
    ti_mod_manifest_t * manifest = &data->manifest;
    ti_module_t * module = data->module;
    CURLcode curle_code;
    mode_t mode = manifest->is_py ? S_IRUSR|S_IWUSR : S_IRWXU;

    /*
     * Access rights file:
     * - Python files:   Read / Write, only by owner.
     * - Binaries:       Read / Write / Execute, only by owner.
     */

    curle_code = gh__download_file(module, manifest->main, mode);
    if (curle_code != CURLE_OK)
    {
        ti_module_set_source_err(
                module, "failed to download `%s` (%s)",
                manifest->main,
                curl_easy_strerror(curle_code));
        return;
    }

    /*
     * Access rights other files: Read / Write, only by owner.
     */
    if (manifest->includes) for (vec_each(manifest->includes, char, fn))
    {
        curle_code = gh__download_file(module, fn, S_IRUSR|S_IWUSR);
        if (curle_code != CURLE_OK)
        {
            ti_module_set_source_err(
                    module, "failed to download `%s` (%s)",
                    fn,
                    curl_easy_strerror(curle_code));
            return;
        }
    }

    /*
     * Includes may be freed, they are no longer required and will not be
     * accessed by the main thread.
     */
    vec_destroy(manifest->includes, (vec_destroy_cb) free);
    manifest->includes = NULL;
}

void ti_mod_github_manifest(uv_work_t * work)
{
    ti_mod_work_t * data = work->data;
    ti_module_t * module = data->module;
    ti_mod_github_t * gh = module->source.github;
    CURLcode curle_code;

    curle_code = gh__download_url(
            gh->manifest_url,
            gh->token_header,
            &data->buf,
            gh__write_mem,
            "raw");

    if (curle_code != CURLE_OK)
    {
        ti_module_set_source_err(
                module, "failed to download "TI_MANIFEST" (%s)",
                curl_easy_strerror(curle_code));
    }
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
