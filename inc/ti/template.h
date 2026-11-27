/*
 * ti/template.h
 */
#ifndef TI_TEMPLATE_H_
#define TI_TEMPLATE_H_

#include <ex.h>
#include <cleri/cleri.h>
#include <ti/template.t.h>
#include <ti/query.h>

int ti_template_build(cleri_node_t *node);
int ti_template_compile(ti_template_t *tmplate, ti_query_t *query, ex_t *e);
void ti_template_destroy(ti_template_t *tmplate);


static inline int ti_template_to_client_pk(
        ti_template_t *template,
        msgpack_packer *pk)
{
    return mp_pack_strn(pk, template->node->str, template->node->len);
}

static inline int ti_template_to_store_pk(
        ti_template_t *template,
        msgpack_packer *pk)
{
    return mp_pack_ext(
            pk,
            MPACK_EXT_TEMPLATE,
            template->node->str, template->node->len);
}

#endif  /* TI_TEMPLATE_H_ */
