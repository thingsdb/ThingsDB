/*
 * ti/verror.h
 */
#ifndef TI_VERROR_H_
#define TI_VERROR_H_

typedef struct ti_verror_s ti_verror_t;

#define TI_VERROR_DEF_CODE -100

#include <ex.h>
#include <inttypes.h>
#include <ti/raw.h>
#include <ti/val.h>
#include <tiinc.h>
#include <util/mpack.h>

void ti_verror_init(void);
ti_verror_t * ti_verror_create(const char * msg, size_t n, int8_t code);
ti_verror_t * ti_verror_from_code(int8_t code);
static inline ti_verror_t * ti_verror_from_raw(int8_t code, ti_raw_t * raw);
static inline ti_verror_t * ti_verror_from_e(ex_t * e);
static inline ti_verror_t * ti_verror_ensure_from_e(ex_t * e);
void ti_verror_to_e(ti_verror_t * verror, ex_t * e);
int ti_verror_check_msg(const char * msg, size_t n, ex_t * e);
const char * ti_verror_type_str(ti_verror_t * verror);
static inline int ti_verror_to_pk(ti_verror_t * verror, msgpack_packer * pk);

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

static inline ti_verror_t * ti_verror_ensure_from_e(ex_t * e)
{
    ti_verror_t * verror = ti_verror_create(e->msg, e->n, (int8_t) e->nr);
    return verror ? verror : ti_verror_from_code(EX_MEMORY);
}

static inline int ti_verror_to_pk(ti_verror_t * verror, msgpack_packer * pk)
{
    return -(
        msgpack_pack_map(pk, 3) ||

        mp_pack_strn(pk, TI_KIND_S_ERROR, 1) ||
        mp_pack_str(pk, ti_verror_type_str(verror)) ||

        mp_pack_str(pk, "error_msg") ||
        mp_pack_strn(pk, verror->msg, verror->msg_n) ||

        mp_pack_str(pk, "error_code") ||
        msgpack_pack_int8(pk, verror->code)
    );
}

#endif  /* TI_VERROR_H_ */
