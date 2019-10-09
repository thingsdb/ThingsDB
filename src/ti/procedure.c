/*
 * ti/procedure.c
 */
#include <assert.h>
#include <ctype.h>
#include <langdef/langdef.h>
#include <stdlib.h>
#include <ti/do.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/ncache.h>
#include <ti/nil.h>
#include <ti/procedure.h>
#include <ti/prop.h>
#include <ti/raw.inline.h>
#include <tiinc.h>
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
        char * def;
        size_t n = 0;
        def = ti_closure_char(procedure->closure, &n);
        if (!def || !(procedure->def = ti_str_create(def, n)))
            procedure->def = (ti_raw_t *) ti_val_empty_str();
        free(def);
    }
    return procedure->def;
}

int ti_procedure_info_to_pk(ti_procedure_t * procedure, msgpack_packer * pk)
{
    ti_raw_t * doc = ti_procedure_doc(procedure);
    ti_raw_t * def = ti_procedure_def(procedure);

    if (msgpack_pack_map(pk,  5) ||

        mp_pack_str(pk, "doc") ||
        mp_pack_strn(pk, doc->data, doc->n) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, procedure->name->data, procedure->name->n) ||

        mp_pack_str(pk, "definition") ||
        mp_pack_strn(pk, def->data, def->n) ||

        mp_pack_str(pk, "with_side_effects") ||
        mp_pack_bool(pk, procedure->closure->flags & TI_VFLAG_CLOSURE_WSE) ||

        mp_pack_str(pk, "arguments") ||
        msgpack_pack_array(pk, procedure->closure->vars->n))
        return -1;

    for (vec_each(procedure->closure->vars, ti_prop_t, prop))
        if (mp_pack_str(pk, prop->name->str))
            return -1;
    return 0;
}

ti_val_t * ti_procedure_as_mpval(ti_procedure_t * procedure)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_procedure_info_to_pk(procedure, &pk))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MP, buffer.size);

    return (ti_val_t *) raw;
}
