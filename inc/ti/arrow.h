/*
 * ti/arrow.h
 */
#ifndef TI_ARROW_H_
#define TI_ARROW_H_

#include <stdint.h>
#include <cleri/cleri.h>
#include <qpack.h>


int ti_arrow_to_packer(cleri_node_t * arrow, qp_packer_t ** packer);

#endif  /* TI_ARROW_H_ */
