/*
 * syncpart.h
 */
#ifndef SYNCPART_H_
#define SYNCPART_H_

#include <qpack.h>

#define SYNCPART_SIZE 131072UL

int syncpart_to_packer(
        qp_packer_t * packer,
        const char * fn,
        off_t offset);

int syncpart_write(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e);

#endif  /* SYNCPART_H_ */
