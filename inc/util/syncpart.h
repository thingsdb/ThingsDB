/*
 * syncpart.h
 */
#ifndef SYNCPART_H_
#define SYNCPART_H_

#include <ex.h>
#include <util/mpack.h>

#ifndef SYNCPART_SIZE
#define SYNCPART_SIZE 131072UL
#endif

int syncpart_to_pk(msgpack_packer * pk, const char * fn, off_t offset);

int syncpart_write(
        const char * fn,
        const unsigned char * data,
        size_t size,
        off_t offset,
        ex_t * e);

#endif  /* SYNCPART_H_ */
