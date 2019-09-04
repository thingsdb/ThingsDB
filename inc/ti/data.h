/*
 * ti/data.h
 */
#ifndef TI_DATA_H_
#define TI_DATA_H_

#include <qpack.h>

typedef struct ti_data_s ti_data_t;

qp_packer_t * ti_data_packer(size_t alloc_size, size_t init_nest_size);
ti_data_t * ti_data_from_packer(qp_packer_t * packer);

struct ti_data_s
{
    size_t n;
    unsigned char data[];
};

#endif  /* TI_DATA_H_ */
