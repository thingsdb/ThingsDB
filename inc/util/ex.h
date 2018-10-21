/*
 * ex.h
 */
#ifndef EX_H_
#define EX_H_

#define EX_ALLOC \
    "allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_MSG_SZ 1024

typedef enum
{
    EX_AUTH_ERROR           =-10,
    EX_FORBIDDEN            =-9,
    EX_INDEX_ERROR          =-8,
    EX_INVALID_DATA         =-7,
    EX_QUERY_ERROR          =-6,
    EX_NODE_ERROR           =-5,
    EX_REQUEST_TIMEOUT      =-4,
    EX_REQUEST_CANCEL       =-3,
    EX_WRITE_UV             =-2,
    EX_MEMORY_ALLOCATION    =-1,
} ex_e;

typedef struct ex_s ex_t;

ex_t * ex_use(void);
void ex_set(ex_t * e, int errnr, const char * errmsg, ...);

struct ex_s
{
    ex_e nr;
    int n;
    char msg[EX_MSG_SZ];
};

#define ex_set_alloc(e__) ex_set((e__), EX_MEMORY_ALLOCATION, EX_ALLOC)

#endif /* EX_H_ */
