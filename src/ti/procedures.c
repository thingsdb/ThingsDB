/*
 * ti/procedures.c
 */
#include <ti/procedures.h>

/*
 * Returns 0 if added, >0 if already exists or <0 in case of an error
 */
int ti_procedures_add(vec_t ** procedures, ti_procedure_t * procedure)
{
    for (vec_each(*procedures, ti_procedure_t, p))
        if (ti_raw_eq(p->name, procedure->name))
            return 1;

    return vec_push(procedures, procedure);
}

ti_procedure_t * ti_procedures_by_name(vec_t * procedures, ti_raw_t * name)
{
    for (vec_each(procedures, ti_procedure_t, p))
        if (ti_raw_eq(p->name, name))
            return p;
    return NULL;
}

ti_procedure_t * ti_procedures_by_strn(
        vec_t * procedures,
        const char * str,
        size_t n)
{
    for (vec_each(procedures, ti_procedure_t, p))
        if (ti_raw_eq_strn(p->name, str, n))
            return p;
    return NULL;
}
