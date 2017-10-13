/*
 * ex.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef EX_H_
#define EX_H_

#define EX_DEF_SZ 128

#define EX_ALLOC \
    "allocation error in '%s' at %s:%d", __func__, __FILE__, __LINE__

typedef enum
{
    EX_REQUEST_TIMEOUT=-3,
    EX_REQUEST_CANCEL,
    EX_WRITE_SOCKET_UV,
    EX_MEMORY_ALLOCATION
} ex_e;

typedef struct ex_s ex_t;

struct ex_s
{
    int errnr;
    size_t sz;
    size_t n;
    char errmsg[];
};

struct ex__s
{
    int errnr;
    size_t sz;
    size_t n;
    char errmsg[EX_DEF_SZ];
} ex__t;

#define ex_ptr(e__) \
    struct ex__s extmp__ = {errnr:0, sz:EX_DEF_SZ, errmsg:""}; \
    ex_t * e__ = (ex_t *) &extmp__

ex_t * ex_new(size_t sz);
int ex_set(ex_t * e, int errnr, const char * errmsg, ...);

#endif /* EX_H_ */
