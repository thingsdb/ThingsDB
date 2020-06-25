/*
 * ti/qbind.h
 */
#ifndef TI_QBIND_H_
#define TI_QBIND_H_

#include <cleri/cleri.h>
#include <ti/qbind.t.h>

void ti_qbind_init(void);
void ti_qbind_probe(ti_qbind_t * qbind, cleri_node_t * nd);

#endif /* TI_QBIND_H_ */
