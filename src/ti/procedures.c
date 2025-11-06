/*
 * ti/procedures.c
 */
#include <ti/procedures.h>
#include <ti/val.inline.h>

/*
 * Returns 0 if added, >0 if already exists or <0 in case of an error
 * Does NOT increment the reference count
 */
int ti_procedures_add(smap_t * procedures, ti_procedure_t * procedure)
{
    switch(smap_add(procedures, procedure->name->str, procedure))
    {
    case SMAP_ERR_ALLOC:
        return -1;
    case SMAP_ERR_EXIST:
        return 1;
    }
    return 0;
}

static int procedures__info_cb(ti_procedure_t * procedure, ti_varr_t * varr)
{
    ti_val_t * mpinfo = ti_procedure_as_mpval(procedure, varr->flags);
    if (!mpinfo)
        return -1;
    VEC_push(varr->vec, mpinfo);
    return 0;
}

ti_varr_t * ti_procedures_info(smap_t * procedures, _Bool with_definition)
{
    uint8_t flags;
    ti_varr_t * varr = ti_varr_create(procedures->n);
    if (!varr)
        return NULL;

    flags = varr->flags;
    varr->flags = with_definition;

    if (smap_values(procedures, (smap_val_cb) procedures__info_cb, varr))
        return ti_val_unsafe_drop((ti_val_t *) varr), NULL;

    varr->flags = flags;
    return varr;
}

static int procedures__pk_cb(ti_procedure_t * procedure, msgpack_packer * pk)
{
    return ti_procedure_info_to_pk(procedure, pk, false);
}

/*
 * Pack procedures without definitions
 */
int ti_procedures_to_pk(smap_t * procedures, msgpack_packer * pk)
{
    if (msgpack_pack_array(pk, procedures->n))
        return -1;

    return smap_values(procedures, (smap_val_cb) procedures__pk_cb, pk);
}

int ti_procedures_rename(
        smap_t * procedures,
        ti_procedure_t * procedure,
        const char * str,
        size_t n)
{
    ti_name_t * name = ti_names_get_slow(str, n);
    if (!name || smap_add(procedures, name->str, procedure))
    {
        ti_name_drop(name);
        return -1;
    }
    (void) smap_pop(procedures, procedure->name->str);

    ti_name_drop(procedure->name);
    procedure->name = name;
    return 0;

}
