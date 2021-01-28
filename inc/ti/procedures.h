/*
 * ti/procedures.h
 */
#ifndef TI_PROCEDURES_H_
#define TI_PROCEDURES_H_

#include <ti/procedure.h>
#include <ti/val.t.h>
#include <ti/varr.t.h>
#include <util/mpack.h>
#include <util/smap.h>

int ti_procedures_add(smap_t * procedures, ti_procedure_t * procedure);
ti_varr_t * ti_procedures_info(smap_t * procedures, _Bool with_definition);
int ti_procedures_to_pk(smap_t * procedures, msgpack_packer * pk);
int ti_procedures_rename(
        smap_t * procedures,
        ti_procedure_t * procedure,
        const char * name,
        size_t n);

static inline void ti_procedures_clear(smap_t * procedures)
{
    smap_clear(procedures, (smap_destroy_cb) ti_procedure_destroy);
}

static inline ti_procedure_t * ti_procedures_pop(
        smap_t * procedures,
        ti_procedure_t * procedure)
{
    return smap_pop(procedures, procedure->name);
}


/* returns a weak reference to a procedure or NULL if not found */
static inline ti_procedure_t * ti_procedures_by_name(
        smap_t * procedures,
        ti_raw_t * name)
{
    return smap_getn(procedures, (const char *) name->data, name->n);
}

static inline  ti_procedure_t * ti_procedures_by_strn(
        smap_t * procedures,
        const char * str,
        size_t n)
{
    return smap_getn(procedures, str, n);
}


#endif /* TI_PROCEDURES_H_ */
