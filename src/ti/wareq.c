/*
 * ti/wareq.c
 */
#include <ti/wareq.h>
#include <assert.h>
#include <stdlib.h>
#include <ti.h>

ti_wareq_t * ti_wareq_create(ti_stream_t * stream)
{
    ti_wareq_t * wareq = malloc(sizeof(ti_wareq_t));
    if (!wareq)
        return NULL;

    wareq->stream = ti_grab(stream);
    wareq->thing_ids = NULL;
    wareq->target = NULL;

    return wareq;
}

void ti_wareq_destroy(ti_wareq_t * wareq)
{
    if (!wareq)
        return;
    vec_destroy(wareq->thing_ids, NULL);
    ti_stream_drop(wareq->stream);
    ti_db_drop(wareq->target);
    free(wareq);
}

int ti_wareq_unpack(ti_wareq_t * wareq, ti_pkg_t * pkg, ex_t * e)
{
    qp_unpacker_t unpacker;
    qp_obj_t key, val;
    vec_t * access_;
    const char * ebad = "invalid watch request, see "TI_DOCS"#watch";

    qp_unpacker_init2(&unpacker, pkg->data, pkg->n, 0);

    if (!qp_is_map(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, ebad);
        goto finish;
    }
    while(qp_is_raw(qp_next(&unpacker, &key)))
    {
        if (qp_is_raw_equal_str(&key, "target"))
        {
            (void) qp_next(&unpacker, &val);
            wareq->target = ti_dbs_get_by_qp_obj(&val, e);
            if (wareq->target)
                ti_incref(wareq->target);
            if (e->nr)
                goto finish;
            continue;
        }

        if (qp_is_raw_equal_str(&key, "things"))
        {
            ssize_t n;
            qp_types_t tp = qp_next(&unpacker, NULL);

            if (!qp_is_array(tp))
            {
                ex_set(e, EX_BAD_DATA, ebad);
                goto finish;
            }

            n = tp == QP_ARRAY_OPEN ? -1 : (ssize_t) tp - QP_ARRAY0;

            wareq->thing_ids = vec_new(n < 0 ? 8 : n);
            if (!wareq->thing_ids)
            {
                ex_set_alloc(e);
                goto finish;
            }

            while(n-- && qp_is_int(qp_next(&unpacker, &val)))
            {
                uintptr_t id = (uintptr_t) val.via.int64;
                if (vec_push(&wareq->thing_ids, id))
                {
                    ex_set_alloc(e);
                    goto finish;
                }
            }
            continue;
        }

        if (!qp_is_map(qp_next(&unpacker, NULL)))
        {
            ex_set(e, EX_BAD_DATA, ebad);
            goto finish;
        }
    }

finish:
    return e->nr;
}
