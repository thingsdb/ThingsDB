/*
 * ti/store/procedures.c
 */
#include <ti.h>
#include <qpack.h>
#include <ti/procedure.h>
#include <ti/store/procedures.h>
#include <util/qpx.h>
#include <util/fx.h>


int ti_store_procedures_store(const vec_t * procedures, const char * fn)
{
    int rc = -1;
    qp_packer_t * packer = qp_packer_create2(1024, 2);
    if (!packer)
        return -1;

    if (qp_add_array(&packer))
        goto stop;

    for (vec_each(procedures, ti_procedure_t, procedure))
        if (qp_add_raw(packer, procedure->def->data, procedure->def->n))
            goto stop;

    if (qp_close_array(packer))
        goto stop;

    rc = fx_write(fn, packer->buffer, packer->len);

stop:
    if (rc)
        log_error("failed to write file: `%s`", fn);
    else
        log_debug("stored procedures to file: `%s`", fn);

    qp_packer_destroy(packer);
    return rc;
}

int ti_store_procedures_restore(vec_t ** procedures, const char * fn)
{
    int rc = -1;
    ex_t e;
    qp_res_t qp_def;
    ti_syntax_t syntax;
    ti_procedure_t * procedure;
    FILE * f = fopen(fn, "r");
    if (!f)
        goto stop;

    ex_init(&e);

    syntax.nd_val_cache_n = 0;
    syntax.flags = *procedures == ti()->procedures
            ? TI_SYNTAX_FLAG_THINGSDB
            : TI_SYNTAX_FLAG_COLLECTION;

    if (!qp_is_array(qp_fnext(f, NULL)))
        goto stop;

    while (qp_is_raw(qp_fnext(f, &qp_def)))
    {
        procedure = ti_procedure_from_strn(
                (char *) qp_def.via.raw->data,
                qp_def.via.raw->n,
                &syntax, &e);

        qp_res_clear(&qp_def);

        if (!procedure)
        {
            log_critical(e.msg);
            continue;
        }

        if (vec_push(procedures, procedure))
            goto stop;
    }

    rc = 0;

stop:
    if (f)
        (void) fclose(f);
    if (rc)
        log_critical("failed to restore from file: `%s`", fn);
    return rc;
}
