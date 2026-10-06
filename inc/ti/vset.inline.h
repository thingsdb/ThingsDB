/*
 * ti/vset.inline.h
 */
#ifndef TI_VSET_INLINE_H_
#define TI_VSET_INLINE_H_

#include <ti/vset.h>
#include <ti/query.t.h>
#include <ti/change.t.h>
#include <ti/closure.t.h>
#include <util/imap.h>


static inline int ti_vset_walk(
        ti_vset_t * vset,
        ti_query_t * query,
        ti_closure_t * closure,
        imap_cb cb,
        void * arg)
{
    if (ti_vset_has_relation(vset))
    {
        if (closure->flags & TI_CLOSURE_FLAG_WSE)
            return imap_walk_cp(
                    vset->imap,
                    cb,
                    arg,
                    (imap_destroy_cb) ti_val_unsafe_drop);
        else if (query->change)
        {
            /* the closure does not required a change, however, within the
             * closure another stored procedure or closure might be called
             * which does require a change; if, in this last case, the query
             * has a change, we could still run the stored closure but this
             * closure in turn must not be allowed to make changes to our set
             * with relations; thus, we enforce no change to force ThingsDB to
             * raise the with-side-effects missing operation error if this
             * will happen; in, by-far most of the cases, this will run fine;
             * see pull request #303
             * */
            int rc;
            ti_change_t * change = query->change;
            query->change = NULL;
            rc = imap_walk(vset->imap, cb, arg);
            query->change = change;
            return rc;
        }
    }
    return imap_walk(vset->imap, cb, arg);
}

#endif  /* TI_VSET_INLINE_H_ */
