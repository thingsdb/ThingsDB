/*
 * ti/verror.h
 */
#ifndef TI_VERROR_H_
#define TI_VERROR_H_

typedef struct ti_verror_s ti_verror_t;

#include <inttypes.h>
#include <ti/val.h>
#include <ti/ex.h>
#include <tiinc.h>

ti_verror_t * ti_verror_create(const char * msg, size_t n, int8_t code);
static inline ti_verror_t * ti_verror_from_e(ex_t * e);
void ti_verror_to_e(ti_verror_t * verror, ex_t * e);
int ti_verror_check_msg(const char * msg, size_t n, ex_t * e);
static inline int ti_verror_to_packer(
        ti_verror_t * verror,
        qp_packer_t ** packer);
static inline int ti_verror_to_file(ti_verror_t * verror, FILE * f);

struct ti_verror_s
{
    uint32_t ref;
    uint8_t tp;
    int8_t code;        /* maps to a ex_enum type */
    uint16_t msg_n;     /* length of msg excluding terminator char */
    char msg[];         /* null terminated, never exceeds EX_MAX_SZ  */
};

static inline ti_verror_t * ti_verror_from_e(ex_t * e)
{
    return ti_verror_create(e->msg, e->n, (int8_t) e->nr);
}

static inline int ti_verror_to_packer(
        ti_verror_t * verror,
        qp_packer_t ** packer)
{
    return -(
        qp_add_map(packer) ||
        qp_add_raw(*packer, (const uchar * ) TI_KIND_S_ERROR, 1) ||
        qp_add_raw(*packer, (const uchar * ) verror->msg, verror->msg_n) ||
        qp_add_raw_from_str(*packer, "code") ||
        qp_add_int(*packer, verror->code) ||
        qp_close_map(*packer)
    );
}

static inline int ti_verror_to_file(ti_verror_t * verror, FILE * f)
{
    return -(
        qp_fadd_type(f, QP_MAP2) ||
        qp_fadd_raw(f, (const uchar * ) TI_KIND_S_REGEX, 1) ||
        qp_fadd_raw(f, (const uchar * ) verror->msg, verror->msg_n) ||
        qp_fadd_raw_from_str(f, "code") ||
        qp_fadd_int(f, verror->code)
    );
}

#endif  /* TI_VERROR_H_ */
