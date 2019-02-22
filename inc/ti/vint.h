/*
 * ti/vint.h
 */
#ifndef TI_VINT_H_
#define TI_VINT_H_

typedef struct ti_vint_s ti_vint_t;

ti_vint_t * ti_vint_create(int64_t i);
static inline void ti_vint_free(ti_vint_t * vint);

struct ti_vint_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    int64_t int_;
};

static inline void ti_vint_free(ti_vint_t * vint)
{
    free(vint);
}

#endif  /* TI_VINT_H_ */
