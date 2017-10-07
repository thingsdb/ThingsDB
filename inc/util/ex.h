/*
 * ex.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef EX_H_
#define EX_H_

#define ERRX_DEF_SZ 128

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
    char errmsg[ERRX_DEF_SZ];
} ex__t;

#define ex_ptr(e__) \
    struct ex__s extmp__ = {errnr:0, sz:ERRX_DEF_SZ, errmsg:""}; \
    ex_t * e__ = (ex_t *) &extmp__

ex_t * ex_new(size_t sz);
int ex_set(ex_t * e, int errnr, const char * errmsg, ...);

#endif /* EX_H_ */
