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
#include <ti/val.inline.h>
#include <tiinc.h>
#include <util/logger.h>

#define PROCEDURE__DEEP_UNSET 255

ti_procedure_t * ti_procedure_create(
        const char * name,
        size_t name_n,
        ti_closure_t * closure,
        uint64_t created_at)
{
    ti_procedure_t * procedure = malloc(sizeof(ti_procedure_t));
    if (!procedure)
        return NULL;

    assert (name);
    assert (closure);

    procedure->name = strndup(name, name_n);
    procedure->name_n = name_n;
    procedure->doc = NULL;
    procedure->def = NULL;
    procedure->closure = closure;
    procedure->created_at = created_at;

    ti_incref(closure);

    if (!procedure->name)
    {
        ti_procedure_destroy(procedure);
        return NULL;
    }
    return procedure;
}

void ti_procedure_destroy(ti_procedure_t * procedure)
{
    if (!procedure)
        return;

    free(procedure->name);
    ti_val_unsafe_drop((ti_val_t *) procedure->closure);
    ti_val_drop((ti_val_t *) procedure->doc);
    ti_val_drop((ti_val_t *) procedure->def);

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
        /* calculate only once */
        procedure->def = ti_closure_def(procedure->closure);
    return procedure->def;
}

int ti_procedure_info_to_pk(
        ti_procedure_t * procedure,
        msgpack_packer * pk,
        _Bool with_definition)
{
    ti_raw_t * doc = ti_procedure_doc(procedure);
    ti_raw_t * def;

    if (msgpack_pack_map(pk, 5 + !!with_definition) ||

        mp_pack_str(pk, "doc") ||
        mp_pack_strn(pk, doc->data, doc->n) ||

        mp_pack_str(pk, "name") ||
        mp_pack_strn(pk, procedure->name, procedure->name_n) ||

        mp_pack_str(pk, "created_at") ||
        msgpack_pack_uint64(pk, procedure->created_at) ||

        (with_definition && (def = ti_procedure_def(procedure)) && (
            mp_pack_str(pk, "definition") ||
            mp_pack_strn(pk, def->data, def->n)
        )) ||

        mp_pack_str(pk, "with_side_effects") ||
        mp_pack_bool(pk, procedure->closure->flags & TI_CLOSURE_FLAG_WSE) ||

        mp_pack_str(pk, "arguments") ||
        msgpack_pack_array(pk, procedure->closure->vars->n))
        return -1;

    for (vec_each(procedure->closure->vars, ti_prop_t, prop))
        if (mp_pack_str(pk, prop->name->str))
            return -1;
    return 0;
}

ti_val_t * ti_procedure_as_mpval(
        ti_procedure_t * procedure,
        _Bool with_definition)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (ti_procedure_info_to_pk(procedure, &pk, with_definition))
    {
        msgpack_sbuffer_destroy(&buffer);
        return NULL;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;
}
