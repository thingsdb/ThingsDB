/*
 * ti/procedures.c
 */
#include <ti/procedures.h>

/*
 * Returns 0 if added, >0 if already exists or <0 in case of an error
 * Does NOT increment the reference count
 */
int ti_procedures_add(vec_t ** procedures, ti_procedure_t * procedure)
{
    for (vec_each(*procedures, ti_procedure_t, p))
        if (ti_raw_eq(p->name, procedure->name))
            return 1;

    return vec_push(procedures, procedure);
}

/* returns a weak reference to a procedure or NULL if not found */
ti_procedure_t * ti_procedures_by_name(vec_t * procedures, ti_raw_t * name)
{
    for (vec_each(procedures, ti_procedure_t, p))
        if (ti_raw_eq(p->name, name))
            return p;
    return NULL;
}

/* returns a weak reference to a procedure or NULL if not found */
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

ti_varr_t * ti_procedures_info(vec_t * procedures)
{
    ti_varr_t * varr = ti_varr_create(procedures->n);
    if (!varr)
        return NULL;

    for (vec_each(procedures, ti_procedure_t, procedure))
    {
        ti_val_t * qpinfo = ti_procedure_info_as_qpval(procedure);
        if (!qpinfo)
        {
            ti_val_drop((ti_val_t *) varr);
            return NULL;
        }
        VEC_push(varr->vec, qpinfo);
    }
    return varr;
}

ti_procedure_t * ti_procedures_pop_name(vec_t * procedures, ti_raw_t * name)
{
    size_t idx = 0;
    for (vec_each(procedures, ti_procedure_t, p), ++idx)
        if (ti_raw_eq(p->name, name))
            return vec_swap_remove(procedures, idx);
    return NULL;
}

ti_procedure_t * ti_procedures_pop_strn(
        vec_t * procedures,
        const char * str,
        size_t n)
{
    size_t idx = 0;
    for (vec_each(procedures, ti_procedure_t, p), ++idx)
        if (ti_raw_eq_strn(p->name, str, n))
            return vec_swap_remove(procedures, idx);
    return NULL;
}
