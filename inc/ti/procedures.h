/*
 * ti/procedures.h
 */
#ifndef TI_PROCEDURES_H_
#define TI_PROCEDURES_H_

#include <ti/procedure.h>
#include <util/vec.h>

int ti_procedures_add(vec_t ** procedures, ti_procedure_t * procedure);
ti_procedure_t * ti_procedures_by_name(vec_t * procedures, ti_raw_t * name);


#endif /* TI_PROCEDURES_H_ */
