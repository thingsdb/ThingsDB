/*
 * ti/procedure.h
 */
#ifndef TI_PROCEDURE_H_
#define TI_PROCEDURE_H_

typedef struct ti_procedure_s ti_procedure_t;

#include <cleri/cleri.h>
#include <inttypes.h>
#include <util/vec.h>
#include <ti/syntax.h>
#include <ti/ex.h>
#include <ti/raw.h>


ti_procedure_t * ti_procedure_from_strn(const char * str, size_t n, ex_t * e);

struct ti_procedure_s
{
    ti_raw_t * name;
    vec_t * arguments;      /* ti_name_t */
    vec_t * nd_val_cache;   /* cached primitives */
    cleri_node_t * node;
    char * body;            /* null terminated body */
    ti_syntax_t syntax;
};


#endif /* TI_PROCEDURE_H_ */
