/*
 * ti/template.h
 */
#ifndef TI_TEMPLATE_H_
#define TI_TEMPLATE_H_

#include <ex.h>
#include <cleri/cleri.h>
#include <ti/template.t.h>
#include <ti/query.h>
#include <ti/qbind.t.h>


ti_template_t *ti_template_create(
        ti_qbind_t *syntax,
        const char *str,
        size_t n,
        ex_t *e);
int ti_template_build(cleri_node_t *node);
int ti_template_compile(ti_template_t *tpl, ti_query_t *query, ex_t *e);
void ti_template_destroy(ti_template_t *tpl);


static inline int ti_template_to_client_pk(
        ti_template_t *tpl,
        msgpack_packer *pk)
{
    return mp_pack_strn(pk, tpl->node->str+1, tpl->node->len-2);
}

static inline int ti_template_to_store_pk(
        ti_template_t *tpl,
        msgpack_packer *pk)
{
    return mp_pack_ext(
            pk,
            MPACK_EXT_TEMPLATE,
            tpl->node->str+1, tpl->node->len-2);
}

static inline size_t ti_template_n(ti_template_t *tpl)
{
    return tpl->node->len-2;
}

static inline int ti_template_try_lock(ti_template_t *tpl, ex_t *e)
{
    if (tpl->flags & TI_TEMPLATE_FLAG_LOCK)
    {
        ex_sets(e, EX_OPERATION, "cannot render a template recursively");
        return -1;
    }
    return (tpl->flags |= TI_TEMPLATE_FLAG_LOCK) & 0;
}

static inline void ti_template_unlock(ti_template_t *tpl)
{
    tpl->flags &= ~TI_TEMPLATE_FLAG_LOCK;
}

#endif  /* TI_TEMPLATE_H_ */
