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

#define EX_MSG_SZ 1024

typedef enum
{
    EX_REQUEST_TIMEOUT=-3,
    EX_REQUEST_CANCEL,
    EX_WRITE_UV,
    EX_MEMORY_ALLOCATION
} ex_e;

typedef struct ex_s ex_t;

struct ex_s
{
    int nr;
    char msg[EX_MSG_SZ];
};

ex_t * ex_use(void);
int ex_set(ex_t * e, int errnr, const char * errmsg, ...);

#define ex_set_alloc(e__) ex_set((e__), TI_PROTO_RUNT_ERR, EX_ALLOC)

#endif /* EX_H_ */
