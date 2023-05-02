/*
 *  This function can almost be replaced with native ThingsDB code, except
 *  in that case a closure is required as argument:
 *
 *       _timeit = |code| {
 *           start = now();
 *           data = code();
 *           time = now() - start;
 *           {
 *               time:,
 *               data:,
 *           };
 *       };
 *
 *       _timeit(|| {
 *           // code to time
 *           2 + 2;
 *       });
 */
#include <ti/fn/fn.h>

static int do__f_timeit(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    static const double d = 1000000000.0;
    const int nargs = fn_get_nargs(nd);
    struct timespec start;
    struct timespec end;
    double timeit;
    ti_val_t * f;
    ti_thing_t * t;

    if (fn_nargs("timeit", DOC_TIMEIT, 1, nargs, e))
        return e->nr;

    clock_gettime(CLOCK_MONOTONIC, &start);

    if (ti_do_statement(query, nd->children, e))
        return e->nr;

    clock_gettime(CLOCK_MONOTONIC, &end);

    timeit = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / d;

    f = (ti_val_t *) ti_vfloat_create(timeit);
    if (!f)
        goto fail0;

    t = ti_thing_o_create(0, 2, query->collection);
    if (!t)
        goto fail1;

    if (!ti_thing_p_prop_add(t, (ti_name_t *) ti_val_data_name(), query->rval))
        goto fail2;  /* leaks data name reference */

    query->rval = (ti_val_t *) t;

    if (!ti_thing_p_prop_add(t, (ti_name_t *) ti_val_time_name(), f))
        goto fail1;  /* skip fail2 now, leaks time name reference */

    return e->nr;

fail2:
    ti_val_unsafe_drop((ti_val_t *) t);
fail1:
    ti_val_unsafe_drop(f);
fail0:
    ex_set_mem(e);
    return e->nr;
}
