/*
 * ti/procedure.h
 */
#ifndef TI_PROCEDURE_H_
#define TI_PROCEDURE_H_

typedef struct ti_procedure_s ti_procedure_t;

enum
{
    TI_RPOCEDURE_FLAG_LOCK   = 1<<0,
    TI_PROCEDURE_FLAG_EVENT  = 1<<1,
};

#include <cleri/cleri.h>
#include <inttypes.h>
#include <util/vec.h>
#include <ti/syntax.h>
#include <ti/ex.h>
#include <ti/query.h>
#include <ti/raw.h>

void ti_procedure_drop(ti_procedure_t * procedure);
ti_procedure_t * ti_procedure_from_raw(
        ti_raw_t * def,
        ti_syntax_t * syntax,
        ex_t * e);
ti_procedure_t * ti_procedure_from_strn(
        const char * str,
        size_t n,
        ti_syntax_t * syntax,
        ex_t * e);
int ti_procedure_call(ti_procedure_t * procedure, ti_query_t * query, ex_t * e);
int ti_procedure_run(ti_query_t * query, ex_t * e);

struct ti_procedure_s
{
    uint32_t ref;
    uint16_t pad0_;
    uint8_t flags;
    uint8_t deep;
    ti_raw_t * def;         /* full procedure definition */
    ti_raw_t * name;
    vec_t * arguments;      /* ti_prop_t */
    vec_t * val_cache;   /* cached primitives */
    cleri_node_t * node;
};


#endif /* TI_PROCEDURE_H_ */
