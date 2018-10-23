/*
 * ti/ex.h
 */
#ifndef EX_H_
#define EX_H_

#define EX_ALLOC \
    "allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_MAX_SZ 1024

typedef enum
{
    EX_AUTH_ERROR           =-105,
    EX_FORBIDDEN            =-104,
    EX_INDEX_ERROR          =-103,
    EX_BAD_DATA             =-102,
    EX_QUERY_ERROR          =-101,
    EX_NODE_ERROR           =-100,

    EX_REQUEST_TIMEOUT      =-4,
    EX_REQUEST_CANCEL       =-3,
    EX_WRITE_UV             =-2,
    EX_MEMORY_ALLOCATION    =-1,

    EX_SUCCESS              =0  /* not an error */
} ex_e;

typedef struct ex_s ex_t;

ex_t * ex_use(void);
void ex_set(ex_t * e, int errnr, const char * errmsg, ...);

struct ex_s
{
    ex_e nr;
    int n;
    char msg[EX_MAX_SZ];
};

#define ex_set_alloc(e__) ex_set((e__), EX_MEMORY_ALLOCATION, EX_ALLOC)

#endif /* EX_H_ */
