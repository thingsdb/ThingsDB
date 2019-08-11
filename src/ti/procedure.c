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
    procedure->doc = NULL;
    procedure->def = NULL;
    procedure->closure = ti_grab(closure);

    return procedure;
}

void ti_procedure_destroy(ti_procedure_t * procedure)
{
    if (!procedure)
        return;

    ti_val_drop((ti_val_t *) procedure->name);
    ti_val_drop((ti_val_t *) procedure->doc);
    ti_val_drop((ti_val_t *) procedure->def);
    ti_val_drop((ti_val_t *) procedure->closure);

    free(procedure);
}

/* may return an empty string but never NULL */
ti_raw_t * ti_procedure_doc(ti_procedure_t * procedure)
{
    if (!procedure->doc)
        procedure->doc = ti_closure_doc(procedure->closure);
    return procedure->doc;
}

/* may return an empty string but never NULL */
ti_raw_t * ti_procedure_def(ti_procedure_t * procedure)
{
    if (!procedure->def)
    {
        uchar * def;
        size_t n = 0;
        def = ti_closure_uchar(procedure->closure, &n);
        if (!def || !(procedure->def = ti_raw_create(def, n)))
            procedure->def = (ti_raw_t *) ti_val_empty_str();
        free(def);
    }
    return procedure->def;
}


int ti_procedure_info_to_packer(
        ti_procedure_t * procedure,
        qp_packer_t ** packer)
{
    ti_raw_t * doc = ti_procedure_doc(procedure);
    ti_raw_t * def = ti_procedure_def(procedure);

    if (qp_add_map(packer) ||
        qp_add_raw_from_str(*packer, TI_KIND_S_INFO) ||
        qp_add_raw(*packer, doc->data, doc->n) ||
        qp_add_raw_from_str(*packer, "name") ||
        qp_add_raw(*packer, procedure->name->data, procedure->name->n) ||
        qp_add_raw_from_str(*packer, "definition") ||
        qp_add_raw(*packer, def->data, def->n) ||
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
