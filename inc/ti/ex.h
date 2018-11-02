/*
 * ti/ex.h
 */
#ifndef TI_EX_H_
#define TI_EX_H_

#define EX_ALLOC_S \
    "allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_INTERNAL_S \
    "internal error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_MAX_SZ 1024

typedef enum
{
    EX_MAX_QUOTA            =-106,
    EX_AUTH_ERROR           =-105,
    EX_FORBIDDEN            =-104,
    EX_INDEX_ERROR          =-103,
    EX_BAD_DATA             =-102,
    EX_QUERY_ERROR          =-101,
    EX_NODE_ERROR           =-100,

    EX_INTERNAL             =-13,
    EX_REQUEST_TIMEOUT      =-4,
    EX_REQUEST_CANCEL       =-3,
    EX_WRITE_UV             =-2,
    EX_ALLOC                =-1,

    EX_SUCCESS              =0  /* not an error */
} ex_enum;

typedef struct ex_s ex_t;

ex_t * ex_use(void);
void ex_set(ex_t * e, ex_enum errnr, const char * errmsg, ...);
const char * ex_str(ex_enum errnr);

struct ex_s
{
    ex_enum nr;
    int n;
    char msg[EX_MAX_SZ];
};

#define ex_set_alloc(e__) ex_set((e__), EX_ALLOC, EX_ALLOC_S)
#define ex_set_internal(e__) ex_set((e__), EX_INTERNAL, EX_INTERNAL_S)

#endif /* TI_EX_H_ */
