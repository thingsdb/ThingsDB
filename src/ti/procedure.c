/*
 * ti/procedure.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <ti/procedure.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/do.h>
#include <ti/ncache.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <tiinc.h>
#include <stdlib.h>
#include <ctype.h>
#include <util/logger.h>

#define PROCEDURE__DEEP_UNSET 255

ti_procedure_t * ti_procedure_create(ti_raw_t * name, ti_closure_t * closure)
{
    ti_procedure_t * procedure = malloc(sizeof(ti_procedure_t));
    if (!procedure)
        return NULL;

    assert (name);
    assert (closure);

    procedure->name = ti_grab(name);
    procedure->closure = ti_grab(closure);

    return procedure;
}

void ti_procedure_destroy(ti_procedure_t * procedure)
{
    if (!procedure)
        return;

    ti_val_drop((ti_val_t *) procedure->name);
    ti_val_drop((ti_val_t *) procedure->closure);

    free(procedure);
}

int ti_procedure_info_to_packer(
        ti_procedure_t * procedure,
        qp_packer_t ** packer)
{
    if (qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, "name") ||
        qp_add_raw(*packer, procedure->name->data, procedure->name->n) ||
        qp_add_raw_from_str(*packer, "with_side_effects") ||
        qp_add_bool(*packer, procedure->closure->flags & TI_VFLAG_CLOSURE_WSE) ||
        qp_add_raw_from_str(*packer, "arguments") ||
        qp_add_array(packer))
        return -1;

    for (vec_each(procedure->closure->vars, ti_prop_t, prop))
        if (qp_add_raw_from_str(*packer, prop->name->str))
            return -1;

    return qp_close_array(*packer) || qp_close_map(*packer) ? -1 : 0;
}

ti_val_t * ti_procedure_info_as_qpval(ti_procedure_t * procedure)
{
    ti_raw_t * rprocedure = NULL;
    qp_packer_t * packer = qp_packer_create2(256, 3);
    if (!packer)
        return NULL;

    if (ti_procedure_info_to_packer(procedure, &packer))
        goto fail;

    rprocedure = ti_raw_from_packer(packer);

fail:
    qp_packer_destroy(packer);
    return (ti_val_t * ) rprocedure;
}
