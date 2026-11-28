/*
 * ti/qbind.h
 */
#ifndef TI_QBIND_H_
#define TI_QBIND_H_

#include <cleri/cleri.h>
#include <ti/qbind.t.h>

void ti_qbind_init(void);
void ti_qbind_probe(ti_qbind_t *qbind, cleri_node_t *nd);

static inline void ti_qbind_reset(ti_qbind_t *qbind, uint8_t flags)
{
    qbind->flags = flags;
    qbind->deep = 0;
    qbind->fut_count = 0;
    qbind->immutable_n = 0;
}

#endif /* TI_QBIND_H_ */
