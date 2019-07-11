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



struct ti_procedure_s
{
    ti_syntax_t syntax;
    vec_t * arguments;      /* ti_name_t */
    vec_t * nd_val_cache;   /* cached primitives */
    cleri_node_t * node;
};


#endif /* TI_PROCEDURE_H_ */
