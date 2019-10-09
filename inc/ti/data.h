/*
 * ti/data.h
 */
#ifndef TI_DATA_H_
#define TI_DATA_H_

#include <ti/name.h>
#include <ti/val.h>

typedef struct ti_data_s ti_data_t;

void ti_data_init(ti_data_t * data, size_t total_n);

struct ti_data_s
{
    size_t n;
    unsigned char data[];
};

#endif  /* TI_DATA_H_ */
