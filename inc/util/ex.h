/*
 * ex.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef EX_H_
#define EX_H_

#define EX_ALLOC \
    "allocation error in '%s' at %s:%d", __func__, __FILE__, __LINE__

typedef enum
{
    EX_REQUEST_TIMEOUT=-3,
    EX_REQUEST_CANCEL,
    EX_WRITE_UV,
    EX_MEMORY_ALLOCATION
} ex_e;

typedef struct ex_s * ex_t;

struct ex_s
{
    int errnr;
    char errmsg[];
};

int ex_set(ex_t * e, int errnr, const char * errmsg, ...);
void ex_destroy(ex_t * e);
#define ex_set_alloc(e__) ex_set((e__), RQL_PROTO_RUNT_ERR, EX_ALLOC)

#endif /* EX_H_ */
