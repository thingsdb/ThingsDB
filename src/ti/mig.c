/*
 * ti/mig.c
 */
#include <ti/mig.h>
#include <ti/raw.inline.h>
#include <util/logger.h>
#include <util/util.h>

ti_mig_t * ti_mig_create(
        uint64_t id,
        time_t ts,
        const char * query,
        size_t query_n,
        const char * info,
        size_t info_n,
        const char * by,
        size_t by_n,
        const char * err_msg,
        size_t err_msg_n)
{
    ti_mig_t * mig = malloc(sizeof(ti_mig_t));
    if (!mig)
        return NULL;

    mig->id = id;
    mig->ts = ts;

    mig->query = ti_str_create(query, query_n);
    if (!mig->query)
        goto fail0;

    mig->info = ti_str_create(info, info_n);
    if (!mig->info)
        goto fail1;

    mig->by = ti_str_create(by, by_n);
    if (!mig->by)
        goto fail2;

    if (err_msg)
    {
        mig->err_msg = ti_str_create(err_msg, err_msg_n);
        if (!mig->err_msg)
            goto fail3;
    }
    else
        mig->err_msg = NULL;

    return mig;
fail3:
    ti_val_unsafe_drop(mig->by);
fail2:
    ti_val_unsafe_drop(mig->info);
fail1:
    ti_val_unsafe_drop(mig->query);
fail0:
    free(mig);
}

ti_mig_t * ti_mig_create_q(
        uint64_t id,
        const char * query,
        ti_raw_t * by)
{
    ti_mig_t * mig = malloc(sizeof(ti_mig_t));
    if (!mig)
        return NULL;

    mig->id = id;
    mig->ts = (time_t) util_now_usec();

    mig->query = ti_str_from_str(query);
    if (!mig->query)
    {
        free(mig);
        return NULL;
    }

    mig->info = ti_val_empty_str();
    mig->by = ti_grab(by);
    mig->err_msg = NULL;
    return mig;
}

void ti_mig_destroy(ti_mig_t * mig)
{
    if (!mig)
        return;
    ti_val_unsafe_drop(mig->query);
    ti_val_unsafe_drop(mig->info);
    ti_val_unsafe_drop(mig->by);
    ti_val_drop(mig->err_msg);
    free(mig);
}

int ti_mig_set_history(vec_t ** migs, _Bool state)
{
    if (state)
    {
        if (*migs)
            return 0;

        *migs = vec_new(8);
        return !!(*migs);
    }
    vec_destroy(*migs, ti_mig_destroy);
    *migs = NULL;
    return 0;
}