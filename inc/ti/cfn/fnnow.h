#ifndef TI_FNNOW_H_
#define TI_FNNOW_H_

/*
 * This file is shared across all scopes and collections
 */

#include <ti/query.h>
#include <langdef/nd.h>
#include <ti/ex.h>
#include <ti/vfloat.h>
#include <cleri/cleri.h>
#include <util/util.h>

#define NOW_DOC_ TI_SEE_DOC("#now")

static int cq__f_now(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    assert (e->nr == 0);
    assert (nd->cl_obj->tp == CLERI_TP_LIST);

    if (!langdef_nd_fun_has_zero_params(nd))
    {
        int nargs = langdef_nd_n_function_params(nd);
        ex_set(e, EX_BAD_DATA,
                "function `now` takes 0 arguments but %d %s given"NOW_DOC_,
                nargs, nargs == 1 ? "was" : "were");
        return e->nr;
    }

    query->rval = (ti_val_t *) ti_vfloat_create(util_now());
    if (!query->rval)
        ex_set_alloc(e);

    return e->nr;
}

#endif /* TI_FNNOW_H_ */
