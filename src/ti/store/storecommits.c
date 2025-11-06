/*
 * ti/store/storecommits.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/commit.h>
#include <ti/commits.h>
#include <ti/raw.inline.h>
#include <ti/store/storecommits.h>
#include <ti/val.inline.h>
#include <util/fx.h>
#include <util/mpack.h>


int ti_store_commits_store(vec_t * commits, const char * fn)
{
    msgpack_packer pk;
    FILE * f = fopen(fn, "w");
    if (!f)
    {
        log_errno_file("cannot open file", errno, fn);
        return -1;
    }

    msgpack_packer_init(&pk, f, msgpack_fbuffer_write);

    if (!commits)
    {
        if (msgpack_pack_map(&pk, 0))
            goto fail;
    }
    else
    {
        if (msgpack_pack_map(&pk, 1) ||
            mp_pack_str(&pk, "commits") ||
            msgpack_pack_array(&pk, commits->n)
        ) goto fail;

        for (vec_each(commits, ti_commit_t, commit))
            if (ti_commit_to_pk(commit, &pk))
                goto fail;
    }

    log_debug("stored commits to file: `%s`", fn);
    goto done;
fail:
    log_error("failed to write file: `%s`", fn);
done:
    if (fclose(f))
    {
        log_errno_file("cannot close file", errno, fn);
        return -1;
    }
    (void) ti_sleep(5);
    return 0;
}

int ti_store_commits_restore(vec_t ** commits, const char * fn)
{
    int rc = -1;
    fx_mmap_t fmap;
    size_t i;
    mp_obj_t obj;
    mp_unp_t up;
    ti_commit_t * commit;

    /* clear existing commits (may exist in the thingsdb scope) */
    ti_commits_destroy(commits);

    if (!fx_file_exist(fn))
    {
        /*
         * TODO: (COMPAT) This check may be removed when we no longer require
         *       backwards compatibility with previous versions of ThingsDB
         *       where the commits file did not exist.
         */
        log_warning(
                "no commits found; "
                "file `%s` is missing",
                fn);
        return ti_store_commits_store(NULL, fn);
    }

    fx_mmap_init(&fmap, fn);
    if (fx_mmap_open(&fmap))  /* fx_mmap_open() is a log function */
        goto fail0;

    mp_unp_init(&up, fmap.data, fmap.n);
    if (mp_next(&up, &obj) != MP_MAP || obj.via.sz > 1)
        goto fail1;

    if (obj.via.sz == 0)
    {
        rc = 0;
        goto done;  /* done, commits disabled */
    }

    if (mp_skip(&up) != MP_STR ||  /* commits */
        mp_next(&up, &obj) != MP_ARR ||
        ti_commits_set_history(commits, true))
        goto fail1;

    for (i = obj.via.sz; i--;)
    {
        commit = ti_commit_from_up(&up);
        if (!commit)
            goto fail1;

        if (vec_push(commits, commit))
            goto fail2;
    }

    rc = 0;
    goto done;

fail2:
    ti_commit_destroy(commit);

done:
fail1:
    if (fx_mmap_close(&fmap))
        rc = -1;
fail0:
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);

    return rc;
}
