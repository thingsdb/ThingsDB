/*
 * ti/verror.h
 */
#ifndef TI_VERROR_H_
#define TI_VERROR_H_

typedef struct ti_verror_s ti_verror_t;

#define TI_VERROR_DEF_CODE -100

#include <inttypes.h>
#include <ti/val.h>
#include <ex.h>
#include <ti/raw.h>
#include <tiinc.h>

void ti_verror_init(void);
ti_verror_t * ti_verror_create(const char * msg, size_t n, int8_t code);
ti_verror_t * ti_verror_from_code(int8_t code);
static inline ti_verror_t * ti_verror_from_raw(int8_t code, ti_raw_t * raw);
static inline ti_verror_t * ti_verror_from_e(ex_t * e);
void ti_verror_to_e(ti_verror_t * verror, ex_t * e);
int ti_verror_check_msg(const char * msg, size_t n, ex_t * e);
const char * ti_verror_type_str(ti_verror_t * verror);
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

static inline ti_verror_t * ti_verror_from_raw(int8_t code, ti_raw_t * raw)
{
    return (raw)
            ? ti_verror_create((const char *) raw->data, raw->n, code)
            : ti_verror_from_code(code);
}

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
        qp_add_raw_from_str(*packer, ti_verror_type_str(verror)) ||
        qp_add_raw_from_str(*packer, "error_msg") ||
        qp_add_raw(*packer, (const uchar * ) verror->msg, verror->msg_n) ||
        qp_add_raw_from_str(*packer, "error_code") ||
        qp_add_int(*packer, verror->code) ||
        qp_close_map(*packer)
    );
}

static inline int ti_verror_to_file(ti_verror_t * verror, FILE * f)
{
    return -(
        qp_fadd_type(f, QP_MAP3) ||
        qp_fadd_raw(f, (const uchar * ) TI_KIND_S_ERROR, 1) ||
        qp_fadd_raw_from_str(f, ti_verror_type_str(verror)) ||
        qp_fadd_raw_from_str(f, "error_msg") ||
        qp_fadd_raw(f, (const uchar * ) verror->msg, verror->msg_n) ||
        qp_fadd_raw_from_str(f, "error_code") ||
        qp_fadd_int(f, verror->code)
    );
}

#endif  /* TI_VERROR_H_ */
