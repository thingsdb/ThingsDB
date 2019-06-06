/*
 * ti/ex.h
 */
#ifndef TI_EX_H_
#define TI_EX_H_

#define EX_ALLOC_S \
    "allocation error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_INTERNAL_S \
    "internal error in `%s` at %s:%d", __func__, __FILE__, __LINE__

#define EX_MAX_SZ 1023


typedef enum
{
    EX_OVERFLOW             =-127,
    EX_ZERO_DIV             =-126,
    EX_MAX_QUOTA            =-125,
    EX_AUTH_ERROR           =-124,
    EX_FORBIDDEN            =-123,
    EX_INDEX_ERROR          =-122,
    EX_BAD_DATA             =-121,
    EX_SYNTAX_ERROR          =-120,
    EX_NODE_ERROR           =-119,
    EX_ASSERT_ERROR         =-118,

    EX_REQUEST_TIMEOUT      =-5,
    EX_REQUEST_CANCEL       =-4,
    EX_WRITE_UV             =-3,
    EX_ALLOC                =-2,
    EX_INTERNAL             =-1,

    EX_SUCCESS              =0,  /* not an error */

    EX_CUSTOM_01, EX_CUSTOM_02, EX_CUSTOM_03, EX_CUSTOM_04,
    EX_CUSTOM_05, EX_CUSTOM_06, EX_CUSTOM_07, EX_CUSTOM_08,
    EX_CUSTOM_09, EX_CUSTOM_0A, EX_CUSTOM_0B, EX_CUSTOM_0C,
    EX_CUSTOM_0D, EX_CUSTOM_0E, EX_CUSTOM_0F, EX_CUSTOM_10,

    EX_CUSTOM_11, EX_CUSTOM_12, EX_CUSTOM_13, EX_CUSTOM_14,
    EX_CUSTOM_15, EX_CUSTOM_16, EX_CUSTOM_17, EX_CUSTOM_18,
    EX_CUSTOM_19, EX_CUSTOM_1A, EX_CUSTOM_1B, EX_CUSTOM_1C,
    EX_CUSTOM_1D, EX_CUSTOM_1E, EX_CUSTOM_1F, EX_CUSTOM_20,
} ex_enum;

typedef struct ex_s ex_t;

ex_t * ex_use(void);
void ex_set(ex_t * e, ex_enum errnr, const char * errmsg, ...);
void ex_setn(ex_t * e, ex_enum errnr, const char * errmsg, size_t n);
const char * ex_str(ex_enum errnr);
_Bool ex_int64_is_errnr(int64_t i);

struct ex_s
{
    ex_enum nr;
    int n;
    char msg[EX_MAX_SZ + 1];    /* 0 terminated message */
};

#define ex_set_alloc(e__) ex_set((e__), EX_ALLOC, EX_ALLOC_S)
#define ex_set_internal(e__) ex_set((e__), EX_INTERNAL, EX_INTERNAL_S)

#endif /* TI_EX_H_ */
