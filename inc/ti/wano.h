/*
 * ti/wano.h
 */
#ifndef TI_WANO_H_
#define TI_WANO_H_

#include <ti/ano.t.h>
#include <ti/wano.t.h>
#include <ti/thing.t.h>

ti_wano_t * ti_wano_create(ti_thing_t * thing, ti_ano_t * ano);
void ti_wano_destroy(ti_wano_t * wano);

#endif /* TI_WANO_H_ */
