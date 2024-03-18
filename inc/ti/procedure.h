/*
 * ti/procedure.h
 */
#ifndef TI_PROCEDURE_H_
#define TI_PROCEDURE_H_

typedef struct ti_procedure_s ti_procedure_t;

#include <cleri/cleri.h>
#include <inttypes.h>
#include <util/vec.h>
#include <ti/qbind.h>
#include <ex.h>
#include <ti/query.h>
#include <ti/raw.h>
#include <ti/closure.h>
#include <ti/val.h>

ti_procedure_t * ti_procedure_create(
        const char * name,
        size_t name_n,
        ti_closure_t * closure,
        uint64_t created_at);
void ti_procedure_mod(
        ti_procedure_t * procedure,
        ti_closure_t * closure,
        uint64_t created_at);
void ti_procedure_destroy(ti_procedure_t * procedure);
int ti_procedure_info_to_pk(
        ti_procedure_t * procedure,
        msgpack_packer * pk,
        _Bool with_definition);
ti_val_t * ti_procedure_as_mpval(
        ti_procedure_t * procedure,
        _Bool with_definition);

struct ti_procedure_s
{
    uint64_t created_at;        /* UNIX time-stamp in seconds */
    char * name;                /* NULL terminated name */
    size_t name_n;              /* size of the name */
    ti_raw_t * doc;             /* documentation, may be NULL */
    ti_raw_t * def;             /* formatted definition, may be NULL */
    ti_closure_t * closure;     /* closure */
};


#endif /* TI_PROCEDURE_H_ */
